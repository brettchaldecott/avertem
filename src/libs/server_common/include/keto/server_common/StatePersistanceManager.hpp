//
// Created by Brett Chaldecott on 2019-06-04.
//

#ifndef KETO_STATEPERSISTANCEMANAGER_HPP
#define KETO_STATEPERSISTANCEMANAGER_HPP

#include <string>
#include <memory>
#include <vector>
#include <mutex>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem/path.hpp>


#include "keto/obfuscate/MetaString.hpp"


namespace keto {
namespace server_common {

class StatePersistanceManager;
typedef std::shared_ptr<StatePersistanceManager> StatePersistanceManagerPtr;

class StatePersistanceManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    class StatePersistanceObject {
    public:
        StatePersistanceObject(StatePersistanceManager* statePersistanceManager, const std::string& key);
        StatePersistanceObject(const StatePersistanceObject& statePersistanceManager) = delete;
        virtual ~StatePersistanceObject();

        operator std::string();
        operator bool();
        operator long();

        void set(const char* value);
        void set(const std::string& value);
        void set(const bool value);
        void set(const long value);

    private:
        StatePersistanceManager* statePersistanceManager;
        std::string key;
    };
    typedef std::shared_ptr<StatePersistanceObject> StatePersistanceObjectPtr;

    class StateMonitor {
    public:
        StateMonitor(StatePersistanceManager* statePersistanceManager);
        StateMonitor(const StateMonitor& stateMonitor) = delete;
        virtual ~StateMonitor();

    private:
        StatePersistanceManager* statePersistanceManager;
    };
    typedef std::shared_ptr<StateMonitor> StateMonitorPtr;


    StatePersistanceManager(const std::string& file);
    StatePersistanceManager(const StatePersistanceManager& statePersistanceManager) = delete;
    virtual ~StatePersistanceManager();

    // state managers
    static StatePersistanceManagerPtr init(const std::string& file);
    static void fin();
    static StatePersistanceManagerPtr getInstance();

    StateMonitorPtr createStateMonitor();
    bool contains(const std::string& key);
    StatePersistanceObjectPtr get(const std::string& key);
    StatePersistanceObject& operator [](const std::string& key);
    bool isDirty();
    void persist();

    boost::filesystem::path getStateFilePath();

private:
    std::mutex classMutex;
    std::string file;
    boost::property_tree::ptree pTree;
    std::map<std::string,StatePersistanceObjectPtr> activeStates;
    bool dirty;


    void setDirty();
    boost::filesystem::path getStateFilePath(const std::string& file);
};


}
}


#endif //KETO_STATEPERSISTANCEMANAGER_HPP
