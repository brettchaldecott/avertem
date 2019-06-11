//
// Created by Brett Chaldecott on 2019-06-04.
//

#include "keto/server_common/StatePersistanceManager.hpp"
#include "keto/environment/EnvironmentManager.hpp"

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>


namespace keto {
namespace server_common {

std::string StatePersistanceManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

StatePersistanceManager::StatePersistanceObject::StatePersistanceObject(StatePersistanceManager* statePersistanceManager, const std::string& key) :
        statePersistanceManager(statePersistanceManager), key(key) {

}

StatePersistanceManager::StatePersistanceObject::~StatePersistanceObject() {
}

StatePersistanceManager::StatePersistanceObject::operator std::string() {
    std::cout << "The string operator is being called" << std::endl;
    return this->statePersistanceManager->pTree.get<std::string>(key);
}

StatePersistanceManager::StatePersistanceObject::operator bool() {
    std::cout << "Get the bool value" << std::endl;
    if (this->statePersistanceManager->pTree.get<std::string>(key) == "true") {
        return true;
    }
    return false;
}

StatePersistanceManager::StatePersistanceObject::operator long() {
    std::cout << "Get the long value" << std::endl;
    return this->statePersistanceManager->pTree.get<long>(key);
}

void StatePersistanceManager::StatePersistanceObject::set(const char* value) {
    std::cout << "Set the string key [" << key << "][" << value << "]" << std::endl;
    this->statePersistanceManager->pTree.put(key,value);
    this->statePersistanceManager->setDirty();
}

void StatePersistanceManager::StatePersistanceObject::set(const std::string& value) {
    std::cout << "Set the string key [" << key << "][" << value << "]" << std::endl;
    this->statePersistanceManager->pTree.put(key,value);
    this->statePersistanceManager->setDirty();
}

void StatePersistanceManager::StatePersistanceObject::set(const bool value) {
    std::cout << "Set the bool key [" << key << "][" << value << "]" << std::endl;
    this->statePersistanceManager->pTree.put(key,value);
    this->statePersistanceManager->setDirty();
}

void StatePersistanceManager::StatePersistanceObject::set(const long value) {
    std::cout << "Set the long key [" << key << "][" << value << "]" << std::endl;
    this->statePersistanceManager->pTree.put(key,value);
    this->statePersistanceManager->setDirty();
}

StatePersistanceManager::StateMonitor::StateMonitor(StatePersistanceManager* statePersistanceManager) : statePersistanceManager(statePersistanceManager) {

}

StatePersistanceManager::StateMonitor::~StateMonitor() {
    this->statePersistanceManager->persist();
}

static StatePersistanceManagerPtr singleton;

StatePersistanceManager::StatePersistanceManager(const std::string& file) : file(file), dirty(false) {

    boost::filesystem::path configDirectoryPath = getStateFilePath(file).remove_filename();
    if (!boost::filesystem::exists(configDirectoryPath)) {
        boost::filesystem::create_directories(configDirectoryPath);
    }
    boost::filesystem::path configFilePath = getStateFilePath(file);
    if (boost::filesystem::exists(configFilePath)) {
        boost::property_tree::ini_parser::read_ini(configFilePath.string(), this->pTree);
    }
}

StatePersistanceManager::~StatePersistanceManager() {

}

// state managers
StatePersistanceManagerPtr StatePersistanceManager::init(const std::string& file) {
    return singleton = StatePersistanceManagerPtr(new StatePersistanceManager(file));
}

void StatePersistanceManager::fin() {
    singleton.reset();
}

StatePersistanceManagerPtr StatePersistanceManager::getInstance() {
    return singleton;
}

StatePersistanceManager::StateMonitorPtr StatePersistanceManager::createStateMonitor() {
    return StatePersistanceManager::StateMonitorPtr(new StatePersistanceManager::StateMonitor(this));
}

bool StatePersistanceManager::contains(const std::string& key) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    if (this->pTree.count(key)) {
        return true;
    }
    return false;
}

StatePersistanceManager::StatePersistanceObjectPtr StatePersistanceManager::get(const std::string& key) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    if (this->activeStates.count(key)) {
        return this->activeStates[key];
    }
    return this->activeStates[key] = StatePersistanceManager::StatePersistanceObjectPtr(new StatePersistanceManager::StatePersistanceObject(this,key));
}

StatePersistanceManager::StatePersistanceObject& StatePersistanceManager::operator [](const std::string& key) {
    std::lock_guard<std::mutex> guard(this->classMutex);
    if (!this->activeStates.count(key)) {
        this->activeStates[key] = StatePersistanceManager::StatePersistanceObjectPtr(new StatePersistanceManager::StatePersistanceObject(this,key));
    }
    return *this->activeStates[key];
}

bool StatePersistanceManager::isDirty() {
    std::lock_guard<std::mutex> guard(this->classMutex);
    return this->dirty;
}

void StatePersistanceManager::persist() {
    std::lock_guard<std::mutex> guard(this->classMutex);
    if (this->dirty) {
        boost::filesystem::path configFilePath = getStateFilePath(file);
        boost::property_tree::ini_parser::write_ini(configFilePath.string(), this->pTree);
        this->dirty=false;
    }
}

boost::filesystem::path StatePersistanceManager::getStateFilePath() {
    return getStateFilePath(this->file);
}

boost::filesystem::path StatePersistanceManager::getStateFilePath(const std::string& file) {
    boost::filesystem::path configPath = file;
    if (keto::environment::EnvironmentManager::getInstance()) {
        configPath =
                keto::environment::EnvironmentManager::getInstance()->getEnv()->getInstallDir() / file;
    }
    return configPath;
}

void StatePersistanceManager::setDirty() {
    this->dirty = true;
}

}
}