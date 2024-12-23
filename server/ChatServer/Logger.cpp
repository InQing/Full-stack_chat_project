#include "Logger.h" 

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

void Logger::SetLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_file_.open(filename, std::ios::out | std::ios::app);
    if (!log_file_) {
        throw std::runtime_error("Failed to open log file");
    }
}

std::string Logger::GetLogLevelString(LogLevel level) {
    switch (level) {
    case LogLevel::INFO_LEVEL: return "INFO";
    case LogLevel::WARNING_LEVEL: return "WARNING";  
    case LogLevel::ERROR_LEVEL: return "ERROR";
    default: return "UNKNOWN";
    }
}

std::string Logger::GetCurrentTimeString() {
    std::time_t now = std::time(nullptr);
    char buf[20];
    std::tm tm_struct;
    localtime_s(&tm_struct, &now);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_struct);
    return buf;
}

Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}
