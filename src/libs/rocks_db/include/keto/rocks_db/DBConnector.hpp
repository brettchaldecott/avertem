/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DBConnector.hpp
 * Author: ubuntu
 *
 * Created on February 21, 2018, 6:54 AM
 */

#ifndef DBCONNECTOR_HPP
#define DBCONNECTOR_HPP

#include <string>
#include <memory>

#include "rocksdb/db.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include "keto/obfuscate/MetaString.hpp"

namespace keto {
namespace rocks_db {

class DBConnector;
typedef std::shared_ptr<DBConnector> DBConnectorPtr;

class DBConnector { 
public:
    static std::string getHeaderVersion() {
        return OBFUSCATED("$Id$");
    };
    static std::string getSourceVersion();

    DBConnector(const std::string& path);
    DBConnector(const DBConnector& orig) = delete;
    virtual ~DBConnector();
    
    rocksdb::TransactionDB* getDB();
private:
    rocksdb::Options options;
    rocksdb::TransactionDBOptions txn_db_options;
    rocksdb::TransactionDB* db;
};


}
};

#endif /* DBCONNECTOR_HPP */

