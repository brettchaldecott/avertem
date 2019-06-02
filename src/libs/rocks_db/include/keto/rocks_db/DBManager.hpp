/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DBManager.hpp
 * Author: ubuntu
 *
 * Created on February 23, 2018, 9:43 AM
 */

#ifndef DBMANAGER_HPP
#define DBMANAGER_HPP

#include <map>
#include <vector>
#include <string>
#include <memory>

#include "keto/rocks_db/DBConnector.hpp"
#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace rocks_db {


class DBManager {
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    DBManager(const std::vector<std::string>& databases, bool purge = false);
    DBManager(const DBManager& orig) = delete;
    virtual ~DBManager();
    
    keto::rocks_db::DBConnectorPtr getConnection(const std::string& database);
    
    
    
private:
    std::map<std::string,keto::rocks_db::DBConnectorPtr> connections;
    
    
};


}
}

#endif /* DBMANAGER_HPP */

