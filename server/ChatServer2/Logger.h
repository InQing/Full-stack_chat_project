#pragma once
#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <memory>
#include <ctime>
#include <sstream>
#include <cstdarg>

class Logger
{
public:
    enum class LogLevel {
        INFO_LEVEL,
        WARNING_LEVEL,
        ERROR_LEVEL,
    };

    static Logger& GetInstance();
    void SetLogFile(const std::string& filename);

    template<typename... Args>
    void log(LogLevel level, const std::string& format, Args... args) {
        std::string message = FormatString(format.c_str(), args...);
        std::string level_str = GetLogLevelString(level);
        std::string time_str = GetCurrentTimeString();

        std::string log_message = "[" + time_str + "]" + "[" + level_str + "] " + message;

        std::cout << log_message << std::endl;

        if (log_file_.is_open()) {
            log_file_ << log_message << std::endl;
        }
    }

    ~Logger();

private:
    std::mutex mutex_;
    std::ofstream log_file_;

    Logger() = default;
    Logger& operator=(const Logger&) = delete;
    Logger(const Logger&) = delete;

    std::string GetLogLevelString(LogLevel level);
    std::string GetCurrentTimeString();

    template<typename... Args>
    std::string FormatString(const char* format, Args... args) {
        int size = std::snprintf(nullptr, 0, format, args...) + 1;
        if (size <= 0) {
            throw std::runtime_error("Error during formatting.");
        }
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format, args...);
        return std::string(buf.get(), buf.get() + size - 1);
    }
};

#define LOGI(format, ...) Logger::GetInstance().log(Logger::LogLevel::INFO_LEVEL, format, ##__VA_ARGS__)
#define LOGW(format, ...) Logger::GetInstance().log(Logger::LogLevel::WARNING_LEVEL, format, ##__VA_ARGS__)
#define LOGE(format, ...) Logger::GetInstance().log(Logger::LogLevel::ERROR_LEVEL, format, ##__VA_ARGS__)

