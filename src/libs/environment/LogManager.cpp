/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Log.cpp
 * Author: ubuntu
 * 
 * Created on January 13, 2018, 6:15 PM
 */

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/support/date_time.hpp>

#include "keto/environment/LogManager.hpp"
#include "include/keto/environment/Constants.hpp"

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;

namespace keto {
    namespace environment {


void keto_formatter(logging::record_view const& rec, logging::formatting_ostream& strm)
{
    // Get the LineID attribute value and put it into the stream
    strm << logging::extract< unsigned int >("LineID", rec) << ": ";

    // The same for the severity level.
    // The simplified syntax is possible if attribute keywords are used.
    strm << "<" << rec[logging::trivial::severity] << "> ";

    // Finally, put the record message to the stream
    strm << rec[expr::smessage];
}

std::string LogManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}
    
LogManager::LogManager(
    const std::shared_ptr<Env>& envPtr,
    const std::shared_ptr<Config>& configPtr) : 
    envPtr(envPtr),configPtr(configPtr) {
    
    // add a log file sink if one is provided
    if (configPtr->getVariablesMap().count(Constants::KETO_LOG_FILE)) {
        boost::filesystem::path logPath = this->envPtr->getInstallDir() / Constants::KETO_LOG_DIR; 
        std::string logFile = logPath.string() + boost::filesystem::path::separator + 
                configPtr->getVariablesMap()[Constants::KETO_LOG_FILE].as<std::string>();
        logging::add_file_log
        (
            keywords::file_name = logFile.c_str(),
            keywords::rotation_size = 10 * 1024 * 1024,
            keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
            keywords::auto_flush = true,
            keywords::format = (
                    expr::stream
                            << "[" << expr::attr< unsigned int >("LineID") << "]["
                            << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                            << "] <" << logging::trivial::severity
                            << "> " << expr::smessage
            )
        );


    }
    
    boost::log::trivial::severity_level logLevel =
        configPtr->getVariablesMap()[Constants::KETO_LOG_LEVEL].as<boost::log::trivial::severity_level>();


    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logLevel
    );

    logging::add_console_log(std::cout, boost::log::keywords::format =(
            expr::stream
                    << "[" << expr::attr< unsigned int >("LineID") << "][" << expr::attr< attrs::current_thread_id::value_type >("ThreadID") << "]["
                    << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                    << "] <" << logging::trivial::severity
                    << "> " << expr::smessage
    ));

    boost::log::add_common_attributes();

}

LogManager::~LogManager() {
}

    
    }
}
