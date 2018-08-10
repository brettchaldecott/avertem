#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp> 

#include <nlohmann/json.hpp>
#include <botan/hex.h>

#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Constants.hpp"

#include "keto/common/MetaInfo.hpp"
#include "keto/common/Log.hpp"
#include "keto/common/Exception.hpp"
#include "keto/common/StringCodec.hpp"

#include "keto/crypto/SecureVectorUtils.hpp"

#include "keto/server_common/VectorUtils.hpp"

#include "keto/key_tools/KeyPairCreator.hpp"
#include "keto/key_tools/ContentDecryptor.hpp"
#include "keto/key_tools/ContentEncryptor.hpp"

#include "keto/tools/Constants.hpp"

#include "keto/crypto/HashGenerator.hpp"

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
            ("encrypt,E", "Encode a file using a private key.")
            ("decrypt,D", "Decrypt a file using a private key.")
            ("hash,H", "Generate a has for a files contents.")
            ("key,K", "Print encoded key.")
            ("private,P", "Print the private key.")
            ("keys,k", po::value<std::string>(),"Key information needed for the encryption")
            ("source,s", po::value<std::string>(),"Source file to encode.")
            ("output,o",po::value<std::string>(), "Output encoded file.")
            ("input,i", po::value<std::string>(), "Input to encode from the command line.");
    
    return optionDescripion;
}

int generateKey(std::shared_ptr<ketoEnv::Config> config,
        boost::program_options::options_description optionDescription) {
    
    if (config->getVariablesMap().count(keto::tools::Constants::KEYS)) {
        std::ifstream ifs(config->getVariablesMap()[keto::tools::Constants::KEYS].as<std::string>());
        nlohmann::json jsonKeys;
        ifs >> jsonKeys;

        keto::crypto::SecureVector secretKey = 
                Botan::hex_decode_locked(jsonKeys[keto::tools::Constants::SECRET_KEY],true);
        keto::crypto::SecureVector encodedKey = 
                Botan::hex_decode_locked(jsonKeys[keto::tools::Constants::ENCODED_KEY],true);
        
        keto::key_tools::KeyPairCreator keyPairCreator(secretKey);
        nlohmann::json json = {
            {keto::tools::Constants::SECRET_KEY, Botan::hex_encode(keyPairCreator.getSecret(),true)},
            {keto::tools::Constants::ENCODED_KEY, Botan::hex_encode(keyPairCreator.getEncodedKey(),true)}
          };

        std::cout << json.dump() << std::endl;
        
    } else {
        
        keto::key_tools::KeyPairCreator keyPairCreator;
        nlohmann::json json = {
            {keto::tools::Constants::SECRET_KEY, Botan::hex_encode(keyPairCreator.getSecret(),true)},
            {keto::tools::Constants::ENCODED_KEY, Botan::hex_encode(keyPairCreator.getEncodedKey(),true)}
          };

        std::cout << json.dump() << std::endl;
    }
    
    return 0;
}

int encryptData(std::shared_ptr<ketoEnv::Config> config,
        boost::program_options::options_description optionDescription) {
    
    if (!config->getVariablesMap().count(keto::tools::Constants::KEYS)) {
        std::cerr << "The keys to encrypt the data must be provided [" << 
                keto::tools::Constants::KEYS << "]" << std::endl;
        return -1;
    }
    
    std::ifstream ifs(config->getVariablesMap()[keto::tools::Constants::KEYS].as<std::string>());
    nlohmann::json jsonKeys;
    ifs >> jsonKeys;
    
    keto::crypto::SecureVector secretKey = 
            Botan::hex_decode_locked(jsonKeys[keto::tools::Constants::SECRET_KEY],true);
    keto::crypto::SecureVector encodedKey = 
            Botan::hex_decode_locked(jsonKeys[keto::tools::Constants::ENCODED_KEY],true);
    
    std::vector<uint8_t> content;
    if (config->getVariablesMap().count(keto::tools::Constants::INPUT)) {
        std::string strContent =
                config->getVariablesMap()[keto::tools::Constants::INPUT].as<std::string>();
        content = keto::server_common::VectorUtils().copyStringToVector(strContent);
    } else if (config->getVariablesMap().count(keto::tools::Constants::SOURCE)){
        std::ifstream ifs(
            config->getVariablesMap()[keto::tools::Constants::SOURCE].as<std::string>());
        if (ifs) {
            int character = -1;
            while ((character = ifs.get()) != -1) {
                content.push_back((uint8_t)character);
            }
        } else {
            std::cerr << "The source file was not read [" << 
                config->getVariablesMap()[keto::tools::Constants::SOURCE].as<std::string>()
                    << "]" << std::endl;
            return -1;
        }
    } else {
        std::cerr << "Content must be provided either [" << 
                keto::tools::Constants::INPUT << "] or ["  << 
                keto::tools::Constants::SOURCE<< "]" << std::endl;
        return -1;
    }
    
    
    keto::key_tools::ContentEncryptor contentEncryptor(secretKey,
            encodedKey,content);
    
    if (config->getVariablesMap().count(keto::tools::Constants::OUTPUT)) {
        std::ofstream output(config->getVariablesMap()[keto::tools::Constants::OUTPUT].as<std::string>());
        output << Botan::hex_encode(contentEncryptor.getEncryptedContent());
        output.close();
    } else {
        std::cout << Botan::hex_encode(contentEncryptor.getEncryptedContent());
    }
    
    
    return 0;
}


int decryptData(std::shared_ptr<ketoEnv::Config> config,
        boost::program_options::options_description optionDescription) {
    
    if (!config->getVariablesMap().count(keto::tools::Constants::KEYS)) {
        std::cerr << "The keys to encrypt the data must be provided [" << 
                keto::tools::Constants::KEYS << "]" << std::endl;
        return -1;
    }
    
    std::ifstream ifs(config->getVariablesMap()[keto::tools::Constants::KEYS].as<std::string>());
    nlohmann::json jsonKeys;
    ifs >> jsonKeys;
    
    keto::crypto::SecureVector secretKey = 
            Botan::hex_decode_locked(jsonKeys[keto::tools::Constants::SECRET_KEY],true);
    keto::crypto::SecureVector encodedKey = 
            Botan::hex_decode_locked(jsonKeys[keto::tools::Constants::ENCODED_KEY],true);
    
    std::vector<uint8_t> content;
    if (config->getVariablesMap().count(keto::tools::Constants::SOURCE)){
        std::ifstream ifs(
            config->getVariablesMap()[keto::tools::Constants::SOURCE].as<std::string>());
        if (ifs) {
            std::string buffer;
            std::copy(std::istream_iterator<uint8_t>(ifs), 
                std::istream_iterator<uint8_t>(),
                std::back_inserter(buffer));
            content = Botan::hex_decode(buffer,true);
        } else {
            std::cerr << "The source file was not read [" << 
                config->getVariablesMap()[keto::tools::Constants::SOURCE].as<std::string>()
                    << "]" << std::endl;
            return -1;
        }
    } else {
        std::cerr << "Content must be provided a source ["  << 
                keto::tools::Constants::SOURCE<< "] to decrypt" << std::endl;
        return -1;
    }
    
    keto::key_tools::ContentDecryptor contentDecryptor(secretKey,
            encodedKey,content);
    
    std::cout << keto::crypto::SecureVectorUtils().copySecureToString(
            contentDecryptor.getContent());
    
    return 0;
    
}

int hashFile(std::shared_ptr<ketoEnv::Config> config,
        boost::program_options::options_description optionDescription) {
    std::vector<uint8_t> content;
    if (config->getVariablesMap().count(keto::tools::Constants::SOURCE)){
        std::ifstream ifs(
            config->getVariablesMap()[keto::tools::Constants::SOURCE].as<std::string>());
        if (ifs) {
            int character = -1;
            while ((character = ifs.get()) != -1) {
                content.push_back((uint8_t)character);
            }
        } else {
            std::cerr << "The source file was not read [" << 
                config->getVariablesMap()[keto::tools::Constants::SOURCE].as<std::string>()
                    << "]" << std::endl;
            return -1;
        }
    } else {
        std::cerr << "Content must be provided the source [" << 
                keto::tools::Constants::SOURCE<< "]" << std::endl;
        return -1;
    }
    
    if (config->getVariablesMap().count(keto::tools::Constants::OUTPUT)){
        std::ofstream output(config->getVariablesMap()[keto::tools::Constants::OUTPUT].as<std::string>());
        output << Botan::hex_encode(keto::crypto::HashGenerator().generateHash(content));
        output.close();
    } else {
        std::cout << Botan::hex_encode(keto::crypto::HashGenerator().generateHash(content));
    }
    
    return 0;
}

int printEncodedKey(std::shared_ptr<ketoEnv::Config> config,
        boost::program_options::options_description optionDescription) {
    
    if (!config->getVariablesMap().count(keto::tools::Constants::KEYS)) {
        std::cerr << "The keys json must provided [" << 
                keto::tools::Constants::KEYS << "]" << std::endl;
        return -1;
    }
    
    std::ifstream ifs(config->getVariablesMap()[keto::tools::Constants::KEYS].as<std::string>());
    nlohmann::json jsonKeys;
    ifs >> jsonKeys;
    
    keto::crypto::SecureVector encodedKey = 
            Botan::hex_decode_locked(jsonKeys[keto::tools::Constants::ENCODED_KEY],true);
    
    std::cout << Botan::hex_encode(encodedKey);
    return 0;
}

int printPrivateKey(std::shared_ptr<ketoEnv::Config> config,
        boost::program_options::options_description optionDescription) {
    
    if (!config->getVariablesMap().count(keto::tools::Constants::KEYS)) {
        std::cerr << "The keys json must provided [" << 
                keto::tools::Constants::KEYS << "]" << std::endl;
        return -1;
    }
    
    std::ifstream ifs(config->getVariablesMap()[keto::tools::Constants::KEYS].as<std::string>());
    nlohmann::json jsonKeys;
    ifs >> jsonKeys;
    
    keto::crypto::SecureVector secretKey = 
            Botan::hex_decode_locked(jsonKeys[keto::tools::Constants::SECRET_KEY],true);
    
    std::cout << Botan::hex_encode(secretKey);
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
        
        if (config->getVariablesMap().count(keto::tools::Constants::GENERATE)) {
            return generateKey(config,optionDescription);
        } else if (config->getVariablesMap().count(keto::tools::Constants::ENCRYPT)) {
            return encryptData(config,optionDescription);
        } else if (config->getVariablesMap().count(keto::tools::Constants::DECRYPT)) {
            return decryptData(config,optionDescription);
        } else if (config->getVariablesMap().count(keto::tools::Constants::HASH)) {
            return hashFile(config,optionDescription);
        } else if (config->getVariablesMap().count(keto::tools::Constants::KEY)) {
            return printEncodedKey(config,optionDescription);
        } else if (config->getVariablesMap().count(keto::tools::Constants::KEY)) {
            return printEncodedKey(config,optionDescription);
        }
        KETO_LOG_INFO << "Keto Tools Executed";
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
