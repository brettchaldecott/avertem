
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp> 

#include "keto/common/MetaInfo.hpp"
#include "keto/common/Log.hpp"
#include "keto/common/Exception.hpp"
#include "keto/common/StringCodec.hpp"
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Constants.hpp"
#include "keto/ssl/RootCertificate.hpp"
#include "keto/cli/Constants.hpp"
#include "keto/session/HttpSession.hpp"

#include "keto/chain_common/TransactionBuilder.hpp"
#include "keto/chain_common/SignedTransactionBuilder.hpp"
#include "keto/chain_common/ActionBuilder.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"

#include "keto/account_utils/AccountGenerator.hpp"

namespace ketoEnv = keto::environment;
namespace ketoCommon = keto::common;
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

boost::program_options::options_description generateOptionDescriptions() {
    boost::program_options::options_description optionDescripion;
    
    optionDescripion.add_options()
            ("help,h", "Print this help message and exit.")
            ("version,v", "Print version information.")
            ("generate,G", "Generate private key, secrete and encoded private key.")
            ("encode,E", "Encode a file using a private key.")
            ("source,s", po::value<std::string>(),"Source file to encode.")
            ("output,o",po::value<std::string>(), "Output encoded file.")
            ("input,i", po::value<std::string>(), "Input to encode from the command line.");
    
    return optionDescripion;
}

int generateKey(std::shared_ptr<ketoEnv::Config> config,
        boost::program_options::options_description optionDescription) {
    
    
    return 0;
}


/**
 * The CLI main file
 * 
 * @param argc
 * @param argv
 * @return 
 */
int main(int argc, char** argv)
{
    try {
        boost::program_options::options_description optionDescription =
                generateOptionDescriptions();
        
        std::shared_ptr<ketoEnv::EnvironmentManager> manager = 
                keto::environment::EnvironmentManager::init(
                keto::environment::Constants::KETO_CLI_CONFIG_FILE,
                optionDescription,argc,argv);
        
        std::shared_ptr<ketoEnv::Config> config = manager->getConfig();
        
        if (config->getVariablesMap().count(ketoEnv::Constants::KETO_VERSION)) {
            std::cout << ketoCommon::MetaInfo::VERSION << std::endl;
            return 0;
        }
        
        if (config->getVariablesMap().count(ketoEnv::Constants::KETO_HELP)) {
            std::cout <<  optionDescription << std::endl;
            return 0;
        }
        
        if (config->getVariablesMap().count(keto::cli::Constants::KETO_TRANSACTION_GEN)) 
        {
            return generateTransaction(config,optionDescription);
        } else if (config->getVariablesMap().count(keto::cli::Constants::KETO_ACCOUNT_GEN)) {
            return generateAccount(config,optionDescription);
        } else if (config->getVariablesMap().count(keto::cli::Constants::KETO_SESSION_GEN)) {
            return generateSession(config);
        }
        KETO_LOG_INFO << "CLI Executed";
    } catch (keto::common::Exception& ex) {
        KETO_LOG_ERROR << "Failed to start because : " << ex.what();
        KETO_LOG_ERROR << "Cause: " << boost::diagnostic_information(ex,true);
        return -1;
    } catch (boost::exception& ex) {
        KETO_LOG_ERROR << "Failed to start because : " << boost::diagnostic_information(ex,true);
        return -1;
    } catch (std::exception& ex) {
        KETO_LOG_ERROR << "Failed to start because : " << ex.what();
        return -1;
    } catch (...) {
        KETO_LOG_ERROR << "Failed to start unknown error.";
        return -1;
    }
}
