
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <chrono>
#include <thread>

#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/process/child.hpp>
#include <boost/asio/signal_set.hpp>


#include "keto/common/Log.hpp"
#include "keto/common/MetaInfo.hpp"
#include "keto/common/Exception.hpp"
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Constants.hpp"
#include "keto/module/ModuleManager.hpp"
#include "keto/module/StateMonitor.hpp"


namespace keto {
    namespace main {

// the keto environment exception base
        KETO_DECLARE_EXCEPTION(MainCommonException, "Main function exited unexpectedly.");

    }
}

namespace ketoEnv = keto::environment;
namespace ketoCommon = keto::common;
namespace ketoModule = keto::module;

boost::program_options::options_description generateOptionDescriptions() {
    boost::program_options::options_description optionDescripion;
    
    optionDescripion.add_options()
            ("help,h", "Print this help message and exit.")
            ("version,v", "Print version information.");
    
    return optionDescripion;
}

class ChildWrapper {
public:
    static void child_signalHandler(int signal) {
        KETO_LOG_ERROR << "####################################################################";
        KETO_LOG_ERROR << "Avertem has been signaled to shut down signal: " << signal;
        KETO_LOG_ERROR << "####################################################################";
        if (ketoModule::ModuleManager::getInstance()) {
            ketoModule::ModuleManager::getInstance()->terminate();
        }
    }

    static void child_signalAbortHandler(int signal) {
        KETO_LOG_ERROR << "####################################################################";
        KETO_LOG_ERROR << "Avertem is ABORTING: this is most likely due to an environmental issue.";
        KETO_LOG_ERROR << "Please check the following : ";
        KETO_LOG_ERROR << "  - enough file handles ";
        KETO_LOG_ERROR << "  - enough memory ";
        KETO_LOG_ERROR << "  - enough disk space ";
        KETO_LOG_ERROR << "  - Keto Shell and Modules are alligned";
        KETO_LOG_ERROR << "####################################################################";
        if (ketoModule::ModuleManager::getInstance()) {
            ketoModule::ModuleManager::getInstance()->terminate();
        }
    }


    int childMain(int argc, char** argv) {
        try {
            keto::module::StateMonitor::init();

            // setup the signal handler
            signal(SIGINT, ChildWrapper::child_signalHandler);
            signal(SIGHUP, ChildWrapper::child_signalHandler);
            signal(SIGTERM, ChildWrapper::child_signalHandler);
            signal(SIGABRT, ChildWrapper::child_signalAbortHandler);

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
            ketoModule::ModuleManager::init();

            KETO_LOG_INFO << "Load the module";
            ketoModule::ModuleManager::getInstance()->load();

            KETO_LOG_INFO << "Monitor the modules";
            ketoModule::ModuleManager::getInstance()->monitor();

            KETO_LOG_INFO << "Terminate the thread that is running in the background";
            ketoModule::ModuleManager::getInstance()->terminate();

            KETO_LOG_INFO << "Wait to unload";
            keto::module::StateMonitor::getInstance()->monitor();

            KETO_LOG_INFO << "Unload the module";
            ketoModule::ModuleManager::getInstance()->unload();

            KETO_LOG_INFO << "Fin the module";
            ketoModule::ModuleManager::fin();

            KETO_LOG_INFO << "KETOD Complete";
            keto::module::StateMonitor::fin();
        } catch (keto::common::Exception& ex) {
            std::cerr << "[keto::common::Exception]Avertem exited unexpectedly : " << ex.what() << std::endl;
            std::cerr << "Cause: " << boost::diagnostic_information(ex,true) << std::endl;
            KETO_LOG_ERROR << "Failed to start because : " << ex.what();
            KETO_LOG_ERROR << "Cause: " << boost::diagnostic_information(ex,true);
            ketoModule::ModuleManager::getInstance()->terminate();
            KETO_LOG_INFO << "Wait to unload";
            if (keto::module::StateMonitor::getInstance()) {
                keto::module::StateMonitor::getInstance()->monitor();
            }
            // unload to force a clean up
            if (ketoModule::ModuleManager::getInstance()) {
                ketoModule::ModuleManager::getInstance()->unload();
            }
            ketoModule::ModuleManager::fin();
            keto::module::StateMonitor::fin();
            return -1;
        } catch (boost::exception& ex) {
            std::cerr << "[boost::exception]Avertem exited unexpectedly : " << boost::diagnostic_information(ex,true) << std::endl;
            KETO_LOG_ERROR << "Failed to start because : " << boost::diagnostic_information(ex,true);
            ketoModule::ModuleManager::getInstance()->terminate();
            KETO_LOG_INFO << "Wait to unload";
            if (keto::module::StateMonitor::getInstance()) {
                keto::module::StateMonitor::getInstance()->monitor();
            }
            // unload to force a clean up
            if (ketoModule::ModuleManager::getInstance()) {
                ketoModule::ModuleManager::getInstance()->unload();
            }
            ketoModule::ModuleManager::fin();
            keto::module::StateMonitor::fin();
            return -1;
        } catch (std::exception& ex) {
            std::cerr << "[std::exception]Avertem exited unexpectedly : " << std::endl;
            std::cerr << "Avertem exited unexpectedly : " << ex.what() << std::endl;
            KETO_LOG_ERROR << "Avertem exited unexpectedly : " << ex.what();
            ketoModule::ModuleManager::getInstance()->terminate();
            KETO_LOG_INFO << "Wait to unload";
            if (keto::module::StateMonitor::getInstance()) {
                keto::module::StateMonitor::getInstance()->monitor();
            }
            // unload to force a clean up
            if (ketoModule::ModuleManager::getInstance()) {
                ketoModule::ModuleManager::getInstance()->unload();
            }
            ketoModule::ModuleManager::fin();
            keto::module::StateMonitor::fin();
            return -1;
        } catch (...) {
            std::cerr << "[unknown]Avertem exited unexpectedly : " << std::endl;
            std::cerr << "Avertem exited unexpectedly" << std::endl;
            KETO_LOG_ERROR << "Avertem exited unexpectedly.";
            ketoModule::ModuleManager::getInstance()->terminate();
            KETO_LOG_INFO << "Wait to unload";
            if (keto::module::StateMonitor::getInstance()) {
                keto::module::StateMonitor::getInstance()->monitor();
            }
            // unload to force a clean up
            if (ketoModule::ModuleManager::getInstance()) {
                ketoModule::ModuleManager::getInstance()->unload();
            }
            ketoModule::ModuleManager::fin();
            keto::module::StateMonitor::fin();
            return -1;
        }
        KETO_LOG_INFO << "Exit";
        return 0;
    }

};

bool terminated = false;
class MainForkManager;
static MainForkManager* currentReference = NULL;

class MainForkManager {
public:
    static void signalHandler(int signal) {
        std::cout << "[signalHandler] child has exited : " << signal << std::endl;
        terminated = true;
        if (currentReference) {
            currentReference->terminate();
        }
    }

    static void signalAbortHandler(int signal) {
        std::cout << "[signalAbortHandler] Unexpected abort" << std::endl;
        terminated = true;
        if (currentReference) {
            currentReference->terminate();
        }
    }

    MainForkManager() {
        currentReference = this;
    }

    ~MainForkManager() {
        currentReference = NULL;
    }

    void forkProcess(int argc, char **argv) {
        pid_t pid = fork();
        if (pid == 0) {
            try {
                ChildWrapper().childMain(argc, argv);
                KETO_LOG_ERROR << "[fork_process] Exit the process";
                return _exit(0);
            } catch (...) {
                KETO_LOG_ERROR << "[fork_process] failed execute correctly unexpected exception. Child is terminating";
                return _exit(-1);
            }

        }
        std::cout << "[fork_process] Running on process : " << pid << std::endl;
        childPtr = std::make_shared<boost::process::child>(pid);
        std::cout << "[fork_process] Wait for process to exit [" << pid << "]" << std::endl;
        childPtr->wait();
        std::cout << "[fork_process] After waiting for process [" << pid << "]" << std::endl;
        if (childPtr->exit_code()) {
            std::cout << "[fork_process] attempt to terminate the process : " <<  childPtr->exit_code() << std::endl;
            BOOST_THROW_EXCEPTION(keto::main::MainCommonException());
        }
        std::cout << "[fork_process] Exit the fork process : " << pid << std::endl;
    }

    void terminate() {
        if (childPtr->running()) {
            kill(childPtr->id(),SIGINT);
        }
    }
private:
    std::shared_ptr<boost::process::child> childPtr;
};

int main(int argc, char** argv)
{
    try {
        // setup the signal handler
        signal(SIGINT, MainForkManager::signalHandler);
        signal(SIGHUP, MainForkManager::signalHandler);
        signal(SIGTERM, MainForkManager::signalHandler);
        signal(SIGABRT, MainForkManager::signalAbortHandler);

        while(!terminated) {
            std::cout << "Start the forked process" << std::endl;
            MainForkManager().forkProcess(argc,argv);
            std::cout << "Forked process completed" << std::endl;
        }
        std::cout << "Start the forked process" << std::endl;
    } catch (keto::common::Exception& ex) {
        std::cerr << "[keto::common::Exception]Keto exited unexpectedly : " << ex.what() << std::endl;
        std::cerr << "Cause: " << boost::diagnostic_information(ex,true) << std::endl;
        return -1;
    } catch (boost::exception& ex) {
        std::cerr << "[boost::exception]Keto exited unexpectedly : " << boost::diagnostic_information(ex,true) << std::endl;
        return -1;
    } catch (std::exception& ex) {
        std::cerr << "[std::exception]Keto exited unexpectedly : " << std::endl;
        std::cerr << "Keto exited unexpectedly : " << ex.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "[unknown]Keto exited unexpectedly : " << std::endl;
        std::cerr << "Keto exited unexpectedly" << std::endl;
        return -1;
    }
}
