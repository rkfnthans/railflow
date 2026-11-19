#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include "CalibrationDataset.hpp"

namespace
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock;
    int passed{};
    int failed{};
    void check(bool value, const char* label)
    {
        if (value) ++passed;
        else { ++failed; std::cerr << "FAILED: " << label << '\n'; }
    }
    class TempDirectory
    {
    public:
        TempDirectory()
        {
            std::string pattern = (std::filesystem::temp_directory_path() / "railflow-calibration-cli-XXXXXX").string();
            if (!::mkdtemp(pattern.data())) throw std::runtime_error("mkdtemp failed");
            path = pattern;
        }
        ~TempDirectory() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
        std::filesystem::path path;
    };

    class Pty
    {
    public:
        Pty()
        {
            fd_ = ::posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);
            if (fd_ < 0) throw std::runtime_error("posix_openpt failed");
            if (::grantpt(fd_) < 0 || ::unlockpt(fd_) < 0 || !::ptsname(fd_))
            { ::close(fd_); throw std::runtime_error("PTY setup failed"); }
            path = ::ptsname(fd_);
        }
        ~Pty() { ::close(fd_); }
        void send(const std::string& bytes)
        {
            std::size_t sent{};
            while (sent < bytes.size())
            {
                const auto size = ::write(fd_, bytes.data()+sent, bytes.size()-sent);
                if (size < 0 && errno == EINTR) continue;
                if (size <= 0) throw std::runtime_error("PTY write failed");
                sent += static_cast<std::size_t>(size);
            }
        }
        std::string path;
    private:
        int fd_{-1};
    };

    class Child
    {
    public:
        Child(const std::string& executable, std::vector<std::string> arguments)
        {
            arguments.insert(arguments.begin(), executable);
            std::vector<char*> argv;
            for (auto& argument : arguments) argv.push_back(argument.data());
            argv.push_back(nullptr);
            int descriptors[2];
            if (::pipe(descriptors) < 0) throw std::runtime_error("pipe failed");
            pid_ = ::fork();
            if (pid_ < 0)
            { ::close(descriptors[0]); ::close(descriptors[1]); throw std::runtime_error("fork failed"); }
            if (pid_ == 0)
            {
                ::close(descriptors[0]);
                if (::dup2(descriptors[1], STDOUT_FILENO) < 0 || ::dup2(descriptors[1], STDERR_FILENO) < 0)
                    ::_exit(126);
                ::close(descriptors[1]);
                ::execv(executable.c_str(), argv.data());
                ::_exit(127);
            }
            ::close(descriptors[1]);
            outputFd_ = descriptors[0];
        }
        ~Child()
        {
            if (pid_ > 0)
            {
                ::kill(pid_, SIGKILL);
                while (::waitpid(pid_, nullptr, 0) < 0 && errno == EINTR) {}
            }
            if (outputFd_ >= 0) ::close(outputFd_);
        }
        Child(const Child&) = delete;
        Child& operator=(const Child&) = delete;
        void pump()
        {
            pollfd output{outputFd_, POLLIN, 0};
            const int ready = ::poll(&output, 1, 10);
            if (ready < 0 && errno != EINTR) throw std::runtime_error("pipe poll failed");
            if (ready > 0 && (output.revents & POLLIN))
            {
                char buffer[2048];
                const auto size = ::read(outputFd_, buffer, sizeof(buffer));
                if (size > 0) text.append(buffer, static_cast<std::size_t>(size));
            }
        }
        void ready()
        {
            const auto deadline = Clock::now()+3s;
            while (text.find("measurement:") == std::string::npos)
            {
                pump();
                if (Clock::now() >= deadline) throw std::runtime_error("CLI startup timeout: "+text);
            }
        }
        void interrupt() { if (::kill(pid_, SIGINT) < 0) throw std::runtime_error("kill failed"); }
        int finish()
        {
            const auto deadline = Clock::now()+3s;
            while (true)
            {
                pump();
                int status{};
                const auto result = ::waitpid(pid_, &status, WNOHANG);
                if (result == pid_)
                {
                    pid_ = -1;
                    // Drain any remaining output after process exit (pipe now reaches EOF).
                    char buffer[2048];
                    for (ssize_t size; (size = ::read(outputFd_, buffer, sizeof(buffer))) > 0;)
                        text.append(buffer, static_cast<std::size_t>(size));
                    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                }
                if (result < 0 && errno != EINTR) throw std::runtime_error("waitpid failed");
                if (Clock::now() >= deadline) throw std::runtime_error("CLI exit timeout: "+text);
            }
        }
        std::string text;
    private:
        pid_t pid_{-1};
        int outputFd_{-1};
    };

    std::vector<std::string> args(const Pty& pty, const std::string& path, std::string settle = "0")
    {
        return {pty.path, "--vehicle", "CAR1", "--weight-g", "500", "--position", "CENTER",
                "--trial", "1", "--settle-ms", std::move(settle), "--duration-sec", "1", "--output", path};
    }
    std::vector<std::string> rows(const std::string& path)
    {
        std::ifstream file(path);
        std::vector<std::string> result;
        for (std::string row; std::getline(file, row);) result.push_back(row);
        return result;
    }
    const std::string first = "RF1,0,0,10,11,12,13,14,15,16,17,18\n";
    const std::string second = "RF1,1,100,20,21,22,23,24,25,26,27,28\n";
}

int main(int argc, char* argv[])
{
    if (argc != 2) return 1;
    try
    {
        TempDirectory directory;
        Pty pty;
        const auto path = (directory.path / "car1.csv").string();
        {
            auto options = args(pty,path);
            options.insert(options.end(), {"--samples","2","--session-id","session,one","--note","test \"note\""});
            Child child(argv[1], options);
            child.ready();
            pty.send("bad RF1\n"+first+second);
            check(child.finish() == 0 && child.text.find("Valid frames: 2") != std::string::npos
                  && child.text.find("Invalid frames: 1") != std::string::npos, "CLI sample collection and invalid exclusion");
        }
        const auto data = rows(path);
        check(data.size() == 3 && data.front() == CalibrationDatasetRecorder::header(), "CLI dataset schema and rows");
        check(data.size() == 3 && data[1].find(",CAR1,500,CENTER,1,10,11,12,13,14,15,16,17,18,First,0,") != std::string::npos,
              "CLI metadata and all nine ADC");
        {
            auto options = args(pty,path);
            options.insert(options.end(), {"--samples","1"});
            Child child(argv[1], options); child.ready(); pty.send(first);
            check(child.finish() == 0 && rows(path).size() == 4, "CLI append without extra header");
        }
        const auto settlePath = (directory.path / "settle.csv").string();
        {
            auto options = args(pty,settlePath,"500");
            options.insert(options.end(), {"--samples","1"});
            Child child(argv[1], options); child.ready(); pty.send(first);
            std::this_thread::sleep_for(600ms);
            pty.send(second);
            check(child.finish() == 0 && child.text.find("Settling lines discarded: 1") != std::string::npos,
                  "CLI real settling interval");
            const auto settled = rows(settlePath);
            check(settled.size() == 2 && settled[1].find(",1,100,CAR1,") != std::string::npos
                  && settled[1].find(",Sequential,0,") != std::string::npos, "settling frame omitted, tracker baseline retained");
        }
        {
            Child child(argv[1], args(pty,(directory.path / "duration.csv").string()));
            child.ready();
            const auto start = Clock::now();
            check(child.finish() == 0 && Clock::now()-start >= 800ms
                  && child.text.find("Valid frames: 0") != std::string::npos, "CLI duration with no input is bounded");
        }
        const auto signalPath = (directory.path / "signal.csv").string();
        {
            Child child(argv[1], args(pty,signalPath)); child.ready(); pty.send(first);
            const auto deadline = Clock::now()+1s;
            while (rows(signalPath).size() < 2 && Clock::now() < deadline) std::this_thread::sleep_for(5ms);
            const auto start = Clock::now();
            child.interrupt();
            check(child.finish() == 0 && Clock::now()-start < 1s
                  && child.text.find("Interrupted: yes") != std::string::npos, "actual SIGINT exits normally");
            check(rows(signalPath).size() == 2, "SIGINT preserves completed CSV row");
        }
        {
            auto options = args(pty,signalPath); options.insert(options.end(), {"--samples","1"});
            Child child(argv[1], options); child.ready(); pty.send(second);
            check(child.finish() == 0 && rows(signalPath).size() == 3, "SIGINT CSV safely reopened for append");
        }
        {
            const auto earlyPath = (directory.path / "early.csv").string();
            Child child(argv[1], args(pty,earlyPath,"1000")); child.ready(); child.interrupt();
            check(child.finish() == 0 && rows(earlyPath).size() == 1, "SIGINT during settling keeps header only");
        }
        {
            const auto rejectedPath = (directory.path / "rejected.csv").string();
            auto options = args(pty,rejectedPath); options[0] = "/nonexistent/serial"; options[2] = "CAR4";
            Child child(argv[1], options);
            check(child.finish() == 1 && child.text.find("Vehicle must be") != std::string::npos
                  && !std::filesystem::exists(rejectedPath), "invalid metadata rejected before port/file operations");
        }
    }
    catch (const std::exception& error)
    {
        ++failed; std::cerr << "Unexpected exception: " << error.what() << '\n';
    }
    std::cout << "Calibration CLI tests: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
