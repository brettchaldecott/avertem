/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VersionProcessor.hpp
 * Author: ubuntu
 *
 * Created on September 28, 2018, 3:40 PM
 */

#ifndef VERSIONPROCESSOR_HPP
#define VERSIONPROCESSOR_HPP

#include <string>
#include <thread>
#include <memory>
#include <deque>
#include <mutex>
#include <condition_variable>

#include "keto/common/MetaInfo.hpp"


namespace keto {
namespace version_manager {

class VersionProcessor;
typedef std::shared_ptr<VersionProcessor> VersionProcessorPtr;
    

class VersionProcessor {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id:$");
    };
    
    static std::string getSourceVersion();

    VersionProcessor();
    VersionProcessor(const VersionProcessor& orig) = delete;
    virtual ~VersionProcessor();
    
    static VersionProcessorPtr init();
    static void fin();
    static VersionProcessorPtr getInstance();
    
    void run();
    void terminate();
    bool isTerminated(); 
    bool isAutoUpdateEnabled();
    
private:
    std::mutex classMutex;
    std::condition_variable stateCondition;
    bool terminated;
    bool autoUpdate;
    std::string ketoHome;
    std::string checkScript;
    
    bool checkTerminated();
    void performUpdateCheck();
    
};

}
}

#endif /* VERSIONPROCESSOR_HPP */

