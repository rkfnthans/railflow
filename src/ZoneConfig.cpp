#include "ZoneConfig.hpp"

std::array<PlatformZone, 3> ZoneConfig::createDefaultZones()
{
    // name, doorX/Y, coreXMin/Max, coreYMin/Max, maxAssignmentDistance
    // 계산 필드는 PlatformZone의 기본 초기값을 사용한다.
    return {{
        {"P1", 200, 560, 100, 300, 440, 600, 360.0},
        {"P2", 600, 560, 500, 700, 440, 600, 360.0},
        {"P3", 1000, 560, 900, 1100, 440, 600, 360.0}
    }};
}
