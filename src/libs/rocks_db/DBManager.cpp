/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DBManager.cpp
 * Author: ubuntu
 * 
 * Created on February 23, 2018, 9:43 AM
 */

#include <iostream>
#include <sstream>

#include <boost/filesystem/path.hpp>

#include "keto/rocks_db/DBManager.hpp"
#include "keto/rocks_db/Exception.hpp"

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Config.hpp"


namespace keto {
namespace rocks_db {

std::string DBManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
DBManager::DBManager(const std::vector<std::string>& databases, bool purge) {
    std::shared_ptr<keto::environment::Config> config = 
            keto::environment::EnvironmentManager::getInstance()->getConfig();
    for (std::string dbName : databases) {
        if (!config->getVariablesMap().count(dbName)) {
            std::stringstream ss;
            ss << "The db name supplied is not configured : " << dbName;
            BOOST_THROW_EXCEPTION(keto::rocks_db::RocksInvalidDBNameException(
                ss.str()));
        }
        boost::filesystem::path dbPath =
                keto::environment::EnvironmentManager::getInstance()->getEnv()->getInstallDir() / 
                config->getVariablesMap()[dbName].as<std::string>();
        if (purge) {
            boost::filesystem::remove_all(dbPath);
        }
        connections[dbName] = DBConnectorPtr(new DBConnector(dbPath.c_str()));
    }
}

DBManager::~DBManager() {
    connections.clear();
}

keto::rocks_db::DBConnectorPtr DBManager::getConnection(const std::string& name) {
    return connections[name];
}


}
}
