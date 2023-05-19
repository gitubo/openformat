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

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLevel(Level severity_){
        severity = static_cast<unsigned int>(severity_);
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
        std::cout << oss.str();
    }

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;    
//    Level severity = Level::DEBUG;
    unsigned int severity = static_cast<unsigned int>(Level::DEBUG);
};

/*
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <memory>
#include <iostream>

#define LOG_LEVEL logging::trivial::info

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

class Logger {
  private:
    logging::sources::severity_logger_mt<logging::trivial::severity_level> logger;
    
    Logger() {
        logging::add_console_log(
            std::cout,
            keywords::format = "[%TimeStamp%] [%Severity%]: %Message%"
        );

        logging::core::get()->set_filter(
            logging::trivial::severity >= LOG_LEVEL
        );

        logging::add_common_attributes();
    }
    
  public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    void log(const std::string& message, logging::trivial::severity_level severity) {
        BOOST_LOG_SEV(logger, severity) << message;
    }

    Logger(Logger const&) = delete;
    void operator=(Logger const&) = delete;
};
*/