
#include <signal.h>
#include <iostream>
#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp> 

#include "keto/common/Log.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/common/Exception.hpp"
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Constants.hpp"
#include "keto/module/ModuleManager.hpp"
#include "keto/module/StateMonitor.hpp"

namespace ketoEnv = keto::environment;
namespace ketoCommon = keto::common;
namespace ketoModule = keto::module;

// the shared ptr for the module manager
std::shared_ptr<ketoModule::ModuleManager> moduleManagerPtr;

boost::program_options::options_description generateOptionDescriptions() {
    boost::program_options::options_description optionDescripion;
    
    optionDescripion.add_options()
            ("help,h", "Print this help message and exit.")
            ("version,v", "Print version information.");
    
    return optionDescripion;
}

void signalHandler(int signal) {
    KETO_LOG_ERROR << "####################################################################";
    KETO_LOG_ERROR << "Avertem has been signaled to shut down signal: " << signal;
    KETO_LOG_ERROR << "####################################################################";
    if (moduleManagerPtr) {
        moduleManagerPtr->terminate();
    }
}

void signalAbortHandler(int signal) {
    KETO_LOG_ERROR << "####################################################################";
    KETO_LOG_ERROR << "Avertem is ABORTING: this is most likely due to an environmental issue.";
    KETO_LOG_ERROR << "Please check the following : ";
    KETO_LOG_ERROR << "  - enough file handles ";
    KETO_LOG_ERROR << "  - enough memory ";
    KETO_LOG_ERROR << "  - enough disk space ";
    KETO_LOG_ERROR << "  - Keto Shell and Modules are alligned";
    KETO_LOG_ERROR << "####################################################################";
    if (moduleManagerPtr) {
        moduleManagerPtr->terminate();
    }
}


int main(int argc, char** argv)
{
    try {
        keto::module::StateMonitor::init();
        // setup the signal handler
        signal(SIGINT, signalHandler);
        signal(SIGHUP, signalHandler);
        signal(SIGTERM, signalHandler);
        signal(SIGABRT, signalAbortHandler);

        // setup the environment
        boost::program_options::options_description optionDescription =
                generateOptionDescriptions();
        std::shared_ptr<ketoEnv::EnvironmentManager> manager = 
                ketoEnv::EnvironmentManager::init(
                ketoEnv::Constants::KETOD_CONFIG_FILE,
                optionDescription,argc,argv);
        
        std::shared_ptr<ketoEnv::Config> config = manager->getConfig();
        
        if (config->getVariablesMap().count(ketoEnv::Constants::KETO_VERSION)) {
            std::cout << ketoCommon::MetaInfo::VERSION << std::endl;
            keto::module::StateMonitor::fin();
            return 0;
        }
        
        if (config->getVariablesMap().count(ketoEnv::Constants::KETO_HELP)) {
            std::cout <<  optionDescription << std::endl;
            keto::module::StateMonitor::fin();
            return 0;
        }
        
        KETO_LOG_INFO << "Instantiate the module manager";
        moduleManagerPtr = ketoModule::ModuleManager::init();
        
        KETO_LOG_INFO << "Load the module";
        moduleManagerPtr->load();

        //KETO_LOG_INFO << "Setup signal handlers";
        //signal(SIGINT, signalHandler);
        //signal(SIGHUP, signalHandler);
        //signal(SIGTERM, signalHandler);
        //signal(SIGABRT, signalAbortHandler);

        KETO_LOG_INFO << "Monitor the modules";
        moduleManagerPtr->monitor();
        KETO_LOG_INFO << "Wait to unload";
        keto::module::StateMonitor::getInstance()->monitor();
        
        KETO_LOG_INFO << "Unload the module";
        moduleManagerPtr->unload();
        
        KETO_LOG_INFO << "KETOD Complete";
        
    } catch (keto::common::Exception& ex) {
        std::cerr << "[keto::common::Exception]Keto exited unexpectedly : " << ex.what() << std::endl;
        std::cerr << "Cause: " << boost::diagnostic_information(ex,true) << std::endl;
        KETO_LOG_ERROR << "Failed to start because : " << ex.what();
        KETO_LOG_ERROR << "Cause: " << boost::diagnostic_information(ex,true);
        // unload to force a clean up
        if (moduleManagerPtr) {
            moduleManagerPtr->unload();
        }
        keto::module::StateMonitor::fin();
        return -1;
    } catch (boost::exception& ex) {
        std::cerr << "[boost::exception]Keto exited unexpectedly : " << boost::diagnostic_information(ex,true) << std::endl;
        KETO_LOG_ERROR << "Failed to start because : " << boost::diagnostic_information(ex,true);
        // unload to force a clean up
        if (moduleManagerPtr) {
            moduleManagerPtr->unload();
        }
        keto::module::StateMonitor::fin();
        return -1;
    } catch (std::exception& ex) {
        std::cerr << "[std::exception]Keto exited unexpectedly : " << std::endl;
        std::cerr << "Keto exited unexpectedly : " << ex.what() << std::endl;
        KETO_LOG_ERROR << "Keto exited unexpectedly : " << ex.what();
        // unload to force a clean up
        if (moduleManagerPtr) {
            moduleManagerPtr->unload();
        }
        keto::module::StateMonitor::fin();
        return -1;
    } catch (...) {
        std::cerr << "[unknown]Keto exited unexpectedly : " << std::endl;
        std::cerr << "Keto exited unexpectedly" << std::endl;
        KETO_LOG_ERROR << "Keto exited unexpectedly.";
        // unload to force a clean up
        if (moduleManagerPtr) {
            moduleManagerPtr->unload();
        }
        keto::module::StateMonitor::fin();
        return -1;
    }
    KETO_LOG_INFO << "Exit";
}
