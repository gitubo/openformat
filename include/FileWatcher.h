#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <thread>

#include "Logger.h"
#include "SchemaCatalog.h"


class FileWatcher {
public:
    FileWatcher(const std::string& path) : path_(path), running_(false), is_ready(false) {}

    bool isReady() {
        return is_ready;
    }

    void StartWatching() {
        running_ = true;
        thread_ = std::thread(&FileWatcher::WatchThread, this);
    }

    void StopWatching() {
        running_ = false;
        thread_.join();
    }

    void loadCatalog(){
        for (auto& file : std::filesystem::recursive_directory_iterator(path_)) {
            if (std::filesystem::is_regular_file(file.path())) {
                std::string filename = file.path().string();
                auto last_write_time = std::filesystem::last_write_time(file.path());
                int analyze_file = 0;
                if(files_.find(filename) == files_.end()){
                    analyze_file = 1;
                    Logger::getInstance().log(std::string("Loading new schema: ") + filename , Logger::Level::INFO);
                } else if(files_[filename] < last_write_time){
                    analyze_file = 2;
                    Logger::getInstance().log(std::string("Reload modified schema: ") + filename , Logger::Level::INFO);                            
                }
                if (analyze_file!=0) {
                    files_[filename] = last_write_time;
                    std::ifstream this_file(filename);
                    if (!this_file.is_open()) {
                        Logger::getInstance().log("Impossible to open file <" + filename + ">", Logger::Level::ERROR);
                        return;
                    }        
                    std::ostringstream copy_stream;
                    try {
                        copy_stream << this_file.rdbuf();                        }
                    catch (const std::exception& e) {
                        Logger::getInstance().log(std::string("Problem reading file: ") + filename + std::string("\n") + e.what(), Logger::Level::ERROR);
                        return;
                    }
                    this_file.close();

                    SchemaCatalog::getInstance().addConfiguration(copy_stream.str(), file.path().stem().string());
                }
            }
        }
        is_ready = true;
    }

    void WatchThread() {
        while (running_) {
/*
            for (auto& file : std::filesystem::recursive_directory_iterator(path_)) {
                if (std::filesystem::is_regular_file(file.path())) {
                    std::string filename = file.path().string();
                    auto last_write_time = std::filesystem::last_write_time(file.path());
                    int analyze_file = 0;
                    if(files_.find(filename) == files_.end()){
                        analyze_file = 1;
                        Logger::getInstance().log(std::string("Loading new schema: ") + filename , Logger::Level::INFO);
                    } else if(files_[filename] < last_write_time){
                        analyze_file = 2;
                        Logger::getInstance().log(std::string("Reload modified schema: ") + filename , Logger::Level::INFO);                            
                    }
                    if (analyze_file!=0) {
                        files_[filename] = last_write_time;
                        std::ifstream this_file(filename);
                        if (!this_file.is_open()) {
                            Logger::getInstance().log("Impossible to open file <" + filename + ">", Logger::Level::ERROR);
                            return;
                        }        
                        std::ostringstream copy_stream;
                        try {
                            copy_stream << this_file.rdbuf();                        }
                        catch (const std::exception& e) {
                            Logger::getInstance().log(std::string("Problem reading file: ") + filename + std::string("\n") + e.what(), Logger::Level::ERROR);
                            return;
                        }
                        this_file.close();

                        SchemaCatalog::getInstance().addConfiguration(copy_stream.str(), file.path().stem().string());
                    }
                }
            }
            is_ready = true;
            */
            
            loadCatalog();
            // Wait for new files
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

private:
    std::string path_;
    std::map<std::string, std::filesystem::file_time_type> files_;
    std::thread thread_;
    bool running_;
    bool is_ready;
};