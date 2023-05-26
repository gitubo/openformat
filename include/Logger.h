#pragma once

#include <iostream>
#include <chrono>
#include <iomanip>

class Logger {
  public:
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    enum class Output {
        STDOUT,
        FILE
    };

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLevel(Level severity_){
        severity = static_cast<unsigned int>(severity_);
    }

    void setOutput(Output output_){
        output = static_cast<unsigned int>(output_);
    }
    
    void log(const std::string& message, Level logLevel) {
        if(static_cast<unsigned int>(logLevel) < severity){ return; }
        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::time_point_cast<std::chrono::microseconds>(now);
        auto value = now_ms.time_since_epoch().count();

        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        auto now_tm = std::localtime(&now_c);

        std::ostringstream oss;
        oss << std::put_time(now_tm, "[%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(6) << (value % 1000000) << "] ";

        switch (logLevel) {
            case Level::DEBUG:
                oss << "[debug] ";
                break;
            case Level::INFO:
                oss << "[info] ";
                break;
            case Level::WARNING:
                oss << "[warning] ";
                break;
            case Level::ERROR:
                oss << "[error] ";
                break;
            case Level::CRITICAL:
                oss << "[critical] ";
                break;
            default:
                std::cout << "UNKNOWN: ";
                break;
        }

        oss << message << std::endl;

        if(output==static_cast<unsigned int>(Output::FILE)){
            std::ofstream log_file("logs", std::ios_base::app);
            log_file << oss.str();
            log_file.close();
        } else {
            std::cout << oss.str();
        }
    }

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;    
    unsigned int severity = static_cast<unsigned int>(Level::DEBUG);
    unsigned int output = static_cast<unsigned int>(Output::STDOUT);
};
