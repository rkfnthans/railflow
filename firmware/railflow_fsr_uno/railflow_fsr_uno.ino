#include <Arduino.h>
#include <stdint.h>

// Keep acquisition preprocessing identical when collecting future calibration data.
namespace AcquisitionConfig
{
    constexpr uint8_t SELECT_PINS[4] = {2, 3, 4, 5}; // S0 (LSB) .. S3 (MSB)
    constexpr uint8_t SIGNAL_PIN = A0;
    // CAR1 FSR1..3 -> C0..2, CAR2 FSR1..3 -> C3..5, CAR3 FSR1..3 -> C6..8.
    // The three sensors in each car are aligned along its 30 cm length.
    constexpr uint8_t CHANNEL_COUNT = 9;
    constexpr unsigned long BAUD_RATE = 115200UL;
    constexpr uint32_t FRAME_PERIOD_MS = 100;
    constexpr unsigned int SETTLING_US = 100;
    constexpr bool DISCARD_FIRST_READ = true;
    constexpr uint8_t SAMPLES_PER_CHANNEL = 4;
    static_assert(CHANNEL_COUNT == 9, "RailFlow must scan exactly 3 cars x 3 FSRs");
    static_assert(SAMPLES_PER_CHANNEL > 0, "At least one ADC sample is required");
    static_assert(FRAME_PERIOD_MS > 0 && FRAME_PERIOD_MS < 0x80000000UL, "Invalid frame period");
}

uint64_t sequence = 0;
uint32_t lastAcquisitionMillis = 0;

void selectMuxChannel(uint8_t channel)
{
    for (uint8_t bit = 0; bit < 4; ++bit)
        digitalWrite(AcquisitionConfig::SELECT_PINS[bit], (channel & (1U << bit)) ? HIGH : LOW);
}

uint16_t readRawChannel(uint8_t channel)
{
    selectMuxChannel(channel);
    delayMicroseconds(AcquisitionConfig::SETTLING_US);
    if (AcquisitionConfig::DISCARD_FIRST_READ) (void)analogRead(AcquisitionConfig::SIGNAL_PIN);
    uint32_t sum = 0;
    for (uint8_t sample = 0; sample < AcquisitionConfig::SAMPLES_PER_CHANNEL; ++sample)
        sum += static_cast<uint16_t>(analogRead(AcquisitionConfig::SIGNAL_PIN));
    // Integer mean, rounded to nearest (half up); stays within Uno ADC 0..1023.
    return static_cast<uint16_t>((sum + AcquisitionConfig::SAMPLES_PER_CHANNEL / 2)
                                / AcquisitionConfig::SAMPLES_PER_CHANNEL);
}

// Uno Print overloads are 32-bit: print uint64 decimal without truncation or heap allocation.
void printSequence(uint64_t value)
{
    char digits[20];
    uint8_t count = 0;
    do
    {
        digits[count++] = static_cast<char>('0' + value % 10);
        value /= 10;
    } while (value != 0);
    while (count != 0) Serial.write(static_cast<uint8_t>(digits[--count]));
}

void setup()
{
    for (uint8_t bit = 0; bit < 4; ++bit)
    {
        pinMode(AcquisitionConfig::SELECT_PINS[bit], OUTPUT);
        digitalWrite(AcquisitionConfig::SELECT_PINS[bit], LOW);
    }
    // MUX EN is physically tied LOW/GND. Each channel has its own FSR divider.
    analogReference(DEFAULT);
    Serial.begin(AcquisitionConfig::BAUD_RATE);
    lastAcquisitionMillis = static_cast<uint32_t>(millis()) - AcquisitionConfig::FRAME_PERIOD_MS;
}

void loop()
{
    const uint32_t now = static_cast<uint32_t>(millis());
    // Unsigned subtraction is safe across millis() wrap. Skip missed periods; no catch-up burst.
    if (static_cast<uint32_t>(now - lastAcquisitionMillis) < AcquisitionConfig::FRAME_PERIOD_MS) return;
    lastAcquisitionMillis = now;
    uint16_t adc[AcquisitionConfig::CHANNEL_COUNT];
    for (uint8_t channel = 0; channel < AcquisitionConfig::CHANNEL_COUNT; ++channel)
        adc[channel] = readRawChannel(channel);
    const uint32_t deviceMillis = static_cast<uint32_t>(millis()); // after all 9 channels
    Serial.print(F("RF1,"));
    printSequence(sequence++);
    Serial.write(',');
    Serial.print(static_cast<unsigned long>(deviceMillis));
    for (uint8_t channel = 0; channel < AcquisitionConfig::CHANNEL_COUNT; ++channel)
    {
        Serial.write(',');
        Serial.print(adc[channel]);
    }
    Serial.write('\n'); // LF only; no diagnostic text on the RF1 stream
}
