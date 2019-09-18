
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

#include <botan/hex.h>
#include <botan/base64.h>
#include <botan/ecdsa.h>

#include <keto/server_common/StringUtils.hpp>


#include "keto/common/MetaInfo.hpp"
#include "keto/common/Log.hpp"
#include "keto/common/Exception.hpp"
#include "keto/common/StringCodec.hpp"
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/environment/Constants.hpp"
#include "keto/ssl/RootCertificate.hpp"
#include "keto/cli/Constants.hpp"
#include "keto/session/HttpSession.hpp"
#include "keto/session/HttpSessionTransactionEncryptor.hpp"
#include "keto/cli/TransactionReader.hpp"
#include "keto/cli/TransactionLoader.hpp"

#include "keto/chain_common/TransactionBuilder.hpp"
#include "keto/chain_common/SignedTransactionBuilder.hpp"
#include "keto/chain_common/ActionBuilder.hpp"
#include "keto/transaction_common/TransactionMessageHelper.hpp"
#include "keto/transaction_common/TransactionTraceBuilder.hpp"
#include "keto/server_common/StringUtils.hpp"
#include "keto/crypto/HashGenerator.hpp"

#include "keto/account_utils/AccountGenerator.hpp"
#include "wally.hpp"
#include "keto/cli/WallyScope.hpp"


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
            ("accgen,A", "Generate an account.")
            ("account_key,K", po::value<std::string>(),"Private key file.")
            ("transgen,T", "Generate a transaction.")
            ("sessiongen,S", "Generate a new session ID.")
            ("word_list,W", "Word list.")
            ("generate_mnemonic_key,G", "Generate a mnemonic key.")
            ("validate_signature,I", "Validate the signature.")
            ("file,f", po::value<std::string>(),"Input file.")
            ("action,a", po::value<std::string>(),"Action Hash or Name.")
            ("parent,p", po::value<std::string>(),"Parent Transaction.")
            ("hash,H", po::value<std::string>(),"Hash.")
            ("signature,i", po::value<std::string>(),"The signature to validate.")
            ("mnemonic_phrase,M", po::value<std::string>(),"The mnemonic phrase.")
            ("mnemonic_pass_phrase,P", po::value<std::string>(),"The mnemonic pass phrase.")
            ("target,t",po::value<std::string>(), "Target Account Hash.")
            ("source,s",po::value<std::string>(), "Source Account Hash.")
            ("mnemonic_language,L", po::value<std::string>(),"language [en...].")
            ("value,V", po::value<long>(), "Value of the transaction.");

    
    return optionDescripion;
}

std::shared_ptr<keto::chain_common::TransactionBuilder> buildTransaction(
        const std::string& contractHash, const std::string& parentHash,
        const std::string& accountHash, const std::string& targetHash,
        const long value) {
    
    std::shared_ptr<keto::chain_common::ActionBuilder> actionPtr =
            keto::chain_common::ActionBuilder::createAction();
    actionPtr->setContract(
            keto::asn1::HashHelper(contractHash,
                keto::common::HEX)).setParent(
            keto::asn1::HashHelper(parentHash,
                keto::common::HEX));
    
    long longValue = value;
    
    std::shared_ptr<keto::chain_common::TransactionBuilder> transactionPtr =
        keto::chain_common::TransactionBuilder::createTransaction();
    transactionPtr->setParent(
            keto::asn1::HashHelper(parentHash,
        keto::common::HEX)).setSourceAccount(
            keto::asn1::HashHelper(accountHash,
        keto::common::HEX)).setTargetAccount(
            keto::asn1::HashHelper(targetHash,
        keto::common::HEX)).setValue(keto::asn1::NumberHelper(longValue));//.addAction(actionPtr);
    
    return transactionPtr;
}

int generateTransaction(std::shared_ptr<ketoEnv::Config> config,
        boost::program_options::options_description optionDescription) {
    // retrieve the host information from the configuration file

    if (!config->getVariablesMap().count(keto::cli::Constants::KETOD_SERVER)) {
        std::cerr << "Please configure the ketod server host information [" << 
                keto::cli::Constants::KETOD_SERVER << "]" << std::endl;
        return -1;
    }
    std::string host = config->getVariablesMap()[keto::cli::Constants::KETOD_SERVER].as<std::string>();

    // retrieve the host information from the configuration file
    if (!config->getVariablesMap().count(keto::cli::Constants::KETOD_PORT)) {
        std::cerr << "Please configure the ketod server port information [" << 
                keto::cli::Constants::KETOD_PORT << "]" << std::endl;
        return -1;
    }
    std::string port = config->getVariablesMap()[keto::cli::Constants::KETOD_PORT].as<std::string>();

    // retrieve the host information from the configuration file
    if (!config->getVariablesMap().count(keto::cli::Constants::PRIVATE_KEY)) {
        std::cerr << "Please configure the private key [" << 
                keto::cli::Constants::PRIVATE_KEY << "]" << std::endl;
        return -1;
    }
    std::string privateKey = config->getVariablesMap()[keto::cli::Constants::PRIVATE_KEY].as<std::string>();

    // retrieve the host information from the configuration file
    if (!config->getVariablesMap().count(keto::cli::Constants::PUBLIC_KEY)) {
        std::cerr << "Please configure the public key [" << 
                keto::cli::Constants::PUBLIC_KEY << "]" << std::endl;
        return -1;
    }
    std::string publicKey = config->getVariablesMap()[keto::cli::Constants::PUBLIC_KEY].as<std::string>();

    // read in the keys
    keto::crypto::KeyLoaderPtr keyLoader(new keto::crypto::KeyLoader(privateKey, publicKey));

    keto::transaction_common::TransactionMessageHelperPtr transactionMessageHelperPtr;

    // check if there is a file
    if (config->getVariablesMap().count(keto::cli::Constants::FILE)) {
        std::string filePath = config->getVariablesMap()[keto::cli::Constants::FILE].as<std::string>();
        boost::filesystem::path transactionPath = filePath;
        keto::cli::TransactionReader transactionReader(transactionPath);

        transactionMessageHelperPtr = keto::cli::TransactionLoader(transactionReader,keyLoader).load();

    } else {
        // retrieve the action hash
        if (!config->getVariablesMap().count(keto::cli::Constants::ACTION)) {
            std::cerr << "Please provide the action name or hash [" <<
                      keto::cli::Constants::ACTION << "]" << std::endl;
            std::cout << optionDescription << std::endl;
            return -1;
        }
        std::string action = config->getVariablesMap()[keto::cli::Constants::ACTION].as<std::string>();
        if (config->getVariablesMap().count(action)) {
            action = config->getVariablesMap()[action].as<std::string>();
        }

        // retrieve the parent hash
        if (!config->getVariablesMap().count(keto::cli::Constants::PARENT)) {
            std::cerr << "Please provide the parent hash [" <<
                      keto::cli::Constants::PARENT << "]" << std::endl;
            std::cout << optionDescription << std::endl;
            return -1;
        }
        std::string parentHash = config->getVariablesMap()[keto::cli::Constants::PARENT].as<std::string>();

        // retrieve source account hash
        if (!config->getVariablesMap().count(keto::cli::Constants::SOURCE_ACCOUNT)) {
            std::cerr << "Please provide the source account [" <<
                      keto::cli::Constants::SOURCE_ACCOUNT << "]" << std::endl;
            std::cout << optionDescription << std::endl;
            return -1;
        }
        std::string sourceAccount = config->getVariablesMap()[keto::cli::Constants::SOURCE_ACCOUNT].as<std::string>();

        // retrieve target account hash
        if (!config->getVariablesMap().count(keto::cli::Constants::TARGET_ACCOUNT)) {
            std::cerr << "Please provide the target account [" <<
                      keto::cli::Constants::TARGET_ACCOUNT << "]" << std::endl;
            std::cout << optionDescription << std::endl;
            return -1;
        }
        std::string targetAccount = config->getVariablesMap()[keto::cli::Constants::TARGET_ACCOUNT].as<std::string>();

        // retrieve transaction value
        if (!config->getVariablesMap().count(keto::cli::Constants::VALUE)) {
            std::cerr << "Please provide the value [" <<
                      keto::cli::Constants::VALUE << "]" << std::endl;
            std::cout << optionDescription << std::endl;
            return -1;
        }
        long value = config->getVariablesMap()[keto::cli::Constants::VALUE].as<long>();


        std::shared_ptr<keto::chain_common::TransactionBuilder> transactionPtr = buildTransaction(
                action, parentHash, sourceAccount, targetAccount, value);

        keto::asn1::PrivateKeyHelper privateKeyHelper;
        privateKeyHelper.setKey(
                Botan::PKCS8::BER_encode(*keyLoader->getPrivateKey()));

        std::shared_ptr<keto::chain_common::SignedTransactionBuilder> signedTransBuild =
                keto::chain_common::SignedTransactionBuilder::createTransaction(
                        privateKeyHelper);


        signedTransBuild->setTransaction(transactionPtr);
        signedTransBuild->sign();

        keto::transaction_common::TransactionWrapperHelperPtr transactionWrapperHelperPtr(
                new keto::transaction_common::TransactionWrapperHelper(
                        *signedTransBuild, keto::asn1::HashHelper(sourceAccount,
                                                                  keto::common::HEX),
                        keto::asn1::HashHelper(targetAccount,
                                               keto::common::HEX)));

        transactionWrapperHelperPtr->addTransactionTrace(
                *keto::transaction_common::TransactionTraceBuilder::createTransactionTrace(sourceAccount, keyLoader));

        transactionMessageHelperPtr = keto::transaction_common::TransactionMessageHelperPtr(
                new keto::transaction_common::TransactionMessageHelper(
                        transactionWrapperHelperPtr));



    }

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::sslv23_client};

    // This holds the root certificate used for verification
    keto::ssl::load_root_certificates(ctx);

    keto::session::HttpSession session(ioc, ctx,
                                       privateKey, publicKey);
    std::string result =
            session.setHost(host).setPort(port).handShake().makeRequest(
                    transactionMessageHelperPtr);

    // Write the message to standard out
    std::cout << result << std::endl;
    return 0;
}

int generateSession(std::shared_ptr<ketoEnv::Config> config) {
    // retrieve the host information from the configuration file
    if (!config->getVariablesMap().count(keto::cli::Constants::KETOD_SERVER)) {
        std::cerr << "Please configure the ketod server host information [" << 
                keto::cli::Constants::KETOD_SERVER << "]" << std::endl;
        return -1;
    }
    std::string host = config->getVariablesMap()[keto::cli::Constants::KETOD_SERVER].as<std::string>();

    // retrieve the host information from the configuration file
    if (!config->getVariablesMap().count(keto::cli::Constants::KETOD_PORT)) {
        std::cerr << "Please configure the ketod server port information [" << 
                keto::cli::Constants::KETOD_PORT << "]" << std::endl;
        return -1;
    }
    std::string port = config->getVariablesMap()[keto::cli::Constants::KETOD_PORT].as<std::string>();

    // retrieve the host information from the configuration file
    if (!config->getVariablesMap().count(keto::cli::Constants::PRIVATE_KEY)) {
        std::cerr << "Please configure the private key [" << 
                keto::cli::Constants::PRIVATE_KEY << "]" << std::endl;
        return -1;
    }
    std::string privateKey = config->getVariablesMap()[keto::cli::Constants::PRIVATE_KEY].as<std::string>();

    // retrieve the host information from the configuration file
    if (!config->getVariablesMap().count(keto::cli::Constants::PUBLIC_KEY)) {
        std::cerr << "Please configure the public key [" << 
                keto::cli::Constants::PUBLIC_KEY << "]" << std::endl;
        return -1;
    }
    std::string publicKey = config->getVariablesMap()[keto::cli::Constants::PUBLIC_KEY].as<std::string>();

    // read in the keys
    keto::crypto::KeyLoader keyLoader(privateKey, publicKey);

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::sslv23_client};

    // This holds the root certificate used for verification
    keto::ssl::load_root_certificates(ctx);

    keto::session::HttpSession session(ioc,ctx,
            privateKey,publicKey);
    std::string result= 
            session.setHost(host).setPort(port).handShake().getSessionId();

    // Write the message to standard out
    std::cout << "Session id : " << result << std::endl;
    return 0;
}


int generateAccount(std::shared_ptr<ketoEnv::Config> config,
        boost::program_options::options_description optionDescription) {
    if (config->getVariablesMap().count(keto::cli::Constants::KETO_ACCOUNT_KEY)) {
        std::string keyPath = config->getVariablesMap()[keto::cli::Constants::KETO_ACCOUNT_KEY].as<std::string>();
        std::cout << ((const std::string)keto::account_utils::AccountGenerator(keyPath)) << std::endl;
    } else {
        std::cout << ((const std::string)keto::account_utils::AccountGenerator()) << std::endl;
    }
    return 0;
}


int printWordList(std::shared_ptr<ketoEnv::Config> config,
                    boost::program_options::options_description optionDescription) {

    if (config->getVariablesMap().count(keto::cli::Constants::MNEMONIC_LANGUAGE)) {
        keto::cli::WallyScope wallyScope;
        std::string language = config->getVariablesMap()[keto::cli::Constants::MNEMONIC_LANGUAGE].as<std::string>();
        words* wordsList;

        bip39_get_wordlist(language.c_str(),&wordsList);
        for (int index = 0; index < BIP39_WORDLIST_LEN; index++) {
            char* word = NULL;
            bip39_get_word(wordsList,index,&word);
            std::cout << word << std::endl;
            wally_free_string(word);
        }


    } else {
        std::cout << "Must provide the language" << std::endl;
        return -1;
    }
    return 0;
}

int generateMnemonicKey(std::shared_ptr<ketoEnv::Config> config,
                 boost::program_options::options_description optionDescription) {
    keto::cli::WallyScope wallyScope;

    // get the word list
    words* wordsList;
    if (config->getVariablesMap().count(keto::cli::Constants::MNEMONIC_LANGUAGE)) {
        keto::cli::WallyScope wallyScope;
        std::string language = config->getVariablesMap()[keto::cli::Constants::MNEMONIC_LANGUAGE].as<std::string>();
        bip39_get_wordlist(language.c_str(),&wordsList);
    } else {
        std::cout << "Must provide the language" << std::endl;
        return -1;
    }

    // retrieve the phrase string
    std::string phrase;
    if (config->getVariablesMap().count(keto::cli::Constants::MNEMONIC_PHRASE)) {
        phrase = config->getVariablesMap()[keto::cli::Constants::MNEMONIC_PHRASE].as<std::string>();
    } else {
        std::cout << "Must provide the mnemonic phrase" << std::endl;
        return -1;
    }
    std::cout << "The phrase is : " << phrase << std::endl;
    keto::server_common::StringVector stringVector = keto::server_common::StringUtils(phrase).tokenize(",");
    phrase = "";
    std::string sep = "";
    for (std::string entry: stringVector) {
        phrase += sep + entry;
        sep = " ";
    }

    std::string passPhrase;
    if (config->getVariablesMap().count(keto::cli::Constants::MNEMONIC_PASS_PHRASE)) {
        passPhrase = config->getVariablesMap()[keto::cli::Constants::MNEMONIC_PASS_PHRASE].as<std::string>();
    }

    // generate the entropy bits

    if (bip39_mnemonic_validate(wordsList, phrase.c_str()) != WALLY_OK) {
        std::cout << "The newmonic phrase is invalid : " << phrase << std::endl;
    }

    unsigned char seed[BIP39_SEED_LEN_512];
    size_t written = 0;
    int result = 0;
    if (passPhrase.empty()) {
        result = bip39_mnemonic_to_seed(phrase.c_str(), NULL, seed, BIP39_SEED_LEN_512, &written);
    } else {
        result = bip39_mnemonic_to_seed(phrase.c_str(), passPhrase.c_str(), seed, BIP39_SEED_LEN_512, &written);
    }

    if (result != WALLY_OK) {
        std::cout << "Failed to add the result" << std::endl;
        return -1;
    }
    std::cout << "Seed : " << Botan::hex_encode(seed,written,true) << std::endl;

    unsigned char entropy[BIP39_ENTROPY_LEN_128];
    size_t entropyWritten = 0;
    bip39_mnemonic_to_bytes(wordsList,phrase.c_str(),entropy,BIP39_ENTROPY_LEN_128,&entropyWritten);

    ext_key* hdKey = nullptr;
    result = bip32_key_from_seed_alloc(seed, written, BIP32_VER_MAIN_PRIVATE, 0, &hdKey);
    if (result != WALLY_OK) {
        std::cout << "HD key failed : " << result << std::endl;
        return -1;
    }

    //unsigned char decompressedKey[EC_PUBLIC_KEY_UNCOMPRESSED_LEN];
    //wally_ec_public_key_decompress(
    //        hdKey->pub_key,
    //        EC_PUBLIC_KEY_LEN,
    //        decompressedKey,
    //        EC_PUBLIC_KEY_UNCOMPRESSED_LEN);

    ext_key* newHdKey = nullptr;
    //std::string path = "m/44'/0'/0'/0/1";

    //uint32_t path[] = {0,0};
    uint32_t path[] = {44 | BIP32_INITIAL_HARDENED_CHILD, 60 | BIP32_INITIAL_HARDENED_CHILD , 0 | BIP32_INITIAL_HARDENED_CHILD,0,0};

    //bip32_key_from_parent_alloc(hdKey,BIP32_INITIAL_HARDENED_CHILD,BIP32_FLAG_KEY_PRIVATE,&newHdKey);
    bip32_key_from_parent_path_alloc(hdKey,path,5,
                                BIP32_FLAG_KEY_PRIVATE,&newHdKey);

    //std::vector<unsigned char> publicBytes(hdKey->pub_key+1,hdKey->pub_key+EC_PUBLIC_KEY_LEN);
    //std::cout << "Public Key  : " << Botan::hex_encode(hdKey->pub_key,EC_PUBLIC_KEY_LEN,true) << std::endl;
    //std::cout << "Private Key : " << Botan::hex_encode(hdKey->priv_key,EC_PRIVATE_KEY_LEN,true) << std::endl;
    //std::cout << "Hash        : " << Botan::hex_encode(hdKey->hash160,20,true) << std::endl;

    std::vector<unsigned char> publicBytes2(newHdKey->pub_key,newHdKey->pub_key+EC_PUBLIC_KEY_LEN);
    keto::crypto::SecureVector accountHash = keto::crypto::HashGenerator().generateHash(publicBytes2);

    std::cout << "Public Key  : " << Botan::hex_encode(newHdKey->pub_key,EC_PUBLIC_KEY_LEN,true) << std::endl;
    std::cout << "Private Key : " << Botan::hex_encode(newHdKey->priv_key,EC_PRIVATE_KEY_LEN,true) << std::endl;
    std::cout << "Account     : " << Botan::hex_encode(accountHash,true) << std::endl;

    /*
    Botan::BigInt bigInt(hdKey->priv_key+1,EC_PRIVATE_KEY_LEN);
    Botan::AutoSeeded_RNG rng;
    Botan::EC_Group ecGroup("secp256k1");
    //ecGroup.get_curve();
    Botan::ECDSA_PrivateKey privateKey(rng, ecGroup, bigInt);
    Botan::ECDSA_PublicKey publicKey(privateKey);
    Botan::ECDSA_PublicKey publicKey2(ecGroup,ecGroup.OS2ECP(hdKey->pub_key,EC_PUBLIC_KEY_LEN));

    unsigned char newPublicKey[EC_PUBLIC_KEY_LEN];
    wally_ec_public_key_from_private_key(hdKey->priv_key+1, EC_PRIVATE_KEY_LEN, newPublicKey, EC_PUBLIC_KEY_LEN);
    std::vector<unsigned char> publicBytes2(newPublicKey+1,newPublicKey+EC_PUBLIC_KEY_LEN);
    std::cout << "Public Key 1.1 : " << Botan::hex_encode(publicBytes2,true) << std::endl;
    std::cout << "Public Key 5.0 : " << publicKey.public_point().get_affine_x().to_hex_string() << std::endl;
    std::cout << "Public Key 6.0 : " << publicKey2.public_point().get_affine_x().to_hex_string() << std::endl;


    //Botan::PointGFp pointGFp(ecGroup.get_curve(),);
    //Botan::ECDSA_PublicKey publicKey2(ecGroup,pointGFp);*/

    //std::cout << "Private Key : " << Botan::hex_encode(hdKey->priv_key,EC_PRIVATE_KEY_LEN,true) << std::endl;

    //keto::crypto::SecureVector secureVector = Botan::PKCS8::BER_encode(privateKey);

    //std::cout << "BER Private Encoded : " << Botan::hex_encode(secureVector,true) << std::endl;

    //std::cout << "Hash : " << Botan::hex_encode(hdKey->hash160,20,true) << std::endl;

    //unsigned char serializedBytes[BIP32_SERIALIZED_LEN];
    //result = bip32_key_serialize(hdKey, BIP32_FLAG_KEY_PRIVATE, serializedBytes, BIP32_SERIALIZED_LEN);

    //if (result != WALLY_OK) {
    //    std::cout << "Failed to serialize the key" << std::endl;
    //    return -1;
    //}

    //std::cout << "HD Private key : " << Botan::hex_encode(serializedBytes,BIP32_SERIALIZED_LEN,true) << std::endl;

    return 0;
}


int verifySignature(std::shared_ptr<ketoEnv::Config> config,
                        boost::program_options::options_description optionDescription) {
    keto::cli::WallyScope wallyScope;

    // get the word list
    words* wordsList;
    if (config->getVariablesMap().count(keto::cli::Constants::MNEMONIC_LANGUAGE)) {
        keto::cli::WallyScope wallyScope;
        std::string language = config->getVariablesMap()[keto::cli::Constants::MNEMONIC_LANGUAGE].as<std::string>();
        bip39_get_wordlist(language.c_str(),&wordsList);
    } else {
        std::cout << "Must provide the language" << std::endl;
        return -1;
    }

    // retrieve the phrase string
    std::string phrase;
    if (config->getVariablesMap().count(keto::cli::Constants::MNEMONIC_PHRASE)) {
        phrase = config->getVariablesMap()[keto::cli::Constants::MNEMONIC_PHRASE].as<std::string>();
    } else {
        std::cout << "Must provide the mnemonic phrase" << std::endl;
        return -1;
    }

    std::cout << "The phrase is : " << phrase << std::endl;
    keto::server_common::StringVector stringVector = keto::server_common::StringUtils(phrase).tokenize(",");
    phrase = "";
    std::string sep = "";
    for (std::string entry: stringVector) {
        phrase += sep + entry;
        sep = " ";
    }

    std::string passPhrase;
    if (config->getVariablesMap().count(keto::cli::Constants::MNEMONIC_PASS_PHRASE)) {
        passPhrase = config->getVariablesMap()[keto::cli::Constants::MNEMONIC_PASS_PHRASE].as<std::string>();
    }

    // retrieve the phrase string
    std::string hash;
    if (config->getVariablesMap().count(keto::cli::Constants::SOURCE_HASH)) {
        hash = config->getVariablesMap()[keto::cli::Constants::SOURCE_HASH].as<std::string>();
    } else {
        std::cout << "Must provide the source hash" << std::endl;
        return -1;
    }

    std::string signature;
    if (config->getVariablesMap().count(keto::cli::Constants::TARGET_SIGNATURE)) {
        signature = config->getVariablesMap()[keto::cli::Constants::TARGET_SIGNATURE].as<std::string>();
    } else {
        std::cout << "Must provide the validate signature" << std::endl;
        return -1;
    }



    // generate the entropy bits

    if (bip39_mnemonic_validate(wordsList, phrase.c_str()) != WALLY_OK) {
        std::cout << "The newmonic phrase is invalid : " << phrase << std::endl;
    }

    unsigned char seed[BIP39_SEED_LEN_512];
    size_t written = 0;
    int result = 0;
    if (passPhrase.empty()) {
        result = bip39_mnemonic_to_seed(phrase.c_str(), NULL, seed, BIP39_SEED_LEN_512, &written);
    } else {
        result = bip39_mnemonic_to_seed(phrase.c_str(), passPhrase.c_str(), seed, BIP39_SEED_LEN_512, &written);
    }

    if (result != WALLY_OK) {
        std::cout << "Failed to add the result" << std::endl;
        return -1;
    }
    std::cout << "Seed : " << Botan::hex_encode(seed,written,true) << std::endl;

    unsigned char entropy[BIP39_ENTROPY_LEN_128];
    size_t entropyWritten = 0;
    bip39_mnemonic_to_bytes(wordsList,phrase.c_str(),entropy,BIP39_ENTROPY_LEN_128,&entropyWritten);

    ext_key* hdKey = nullptr;
    result = bip32_key_from_seed_alloc(seed, written, BIP32_VER_MAIN_PRIVATE, 0, &hdKey);
    if (result != WALLY_OK) {
        std::cout << "HD key failed : " << result << std::endl;
        return -1;
    }

    //unsigned char decompressedKey[EC_PUBLIC_KEY_UNCOMPRESSED_LEN];
    //wally_ec_public_key_decompress(
    //        hdKey->pub_key,
    //        EC_PUBLIC_KEY_LEN,
    //        decompressedKey,
    //        EC_PUBLIC_KEY_UNCOMPRESSED_LEN);

    ext_key* newHdKey = nullptr;
    //std::string path = "m/44'/0'/0'/0/1";

    //uint32_t path[] = {0,0};
    uint32_t path[] = {44 | BIP32_INITIAL_HARDENED_CHILD, 60 | BIP32_INITIAL_HARDENED_CHILD , 0 | BIP32_INITIAL_HARDENED_CHILD,0,0};

    //bip32_key_from_parent_alloc(hdKey,BIP32_INITIAL_HARDENED_CHILD,BIP32_FLAG_KEY_PRIVATE,&newHdKey);
    bip32_key_from_parent_path_alloc(hdKey,path,5,
                                     BIP32_FLAG_KEY_PRIVATE,&newHdKey);

    //std::vector<unsigned char> publicBytes(hdKey->pub_key+1,hdKey->pub_key+EC_PUBLIC_KEY_LEN);
    //std::cout << "Public Key  : " << Botan::hex_encode(hdKey->pub_key,EC_PUBLIC_KEY_LEN,true) << std::endl;
    //std::cout << "Private Key : " << Botan::hex_encode(hdKey->priv_key,EC_PRIVATE_KEY_LEN,true) << std::endl;
    //std::cout << "Hash        : " << Botan::hex_encode(hdKey->hash160,20,true) << std::endl;

    std::vector<unsigned char> publicBytes2(newHdKey->pub_key,newHdKey->pub_key+EC_PUBLIC_KEY_LEN);
    keto::crypto::SecureVector accountHash = keto::crypto::HashGenerator().generateHash(publicBytes2);

    std::cout << "Public Key  : " << Botan::hex_encode(newHdKey->pub_key,EC_PUBLIC_KEY_LEN,true) << std::endl;
    std::cout << "Private Key : " << Botan::hex_encode(newHdKey->priv_key,EC_PRIVATE_KEY_LEN,true) << std::endl;
    std::cout << "Account     : " << Botan::hex_encode(accountHash,true) << std::endl;

    std::vector<unsigned char> hashBytes = Botan::hex_decode(hash);
    std::vector<unsigned char> signatureBytes = Botan::hex_decode(signature);

    if (WALLY_OK ==
        wally_ec_sig_verify(newHdKey->pub_key,EC_PUBLIC_KEY_LEN,hashBytes.data(),hashBytes.size(),EC_FLAG_ECDSA,signatureBytes.data(),signatureBytes.size())) {
        std::cout << "The signature is valid" << std::endl;
    } else {
        std::cout << "The signature is invalid" << std::endl;
    }

    /*
    Botan::BigInt bigInt(hdKey->priv_key+1,EC_PRIVATE_KEY_LEN);
    Botan::AutoSeeded_RNG rng;
    Botan::EC_Group ecGroup("secp256k1");
    //ecGroup.get_curve();
    Botan::ECDSA_PrivateKey privateKey(rng, ecGroup, bigInt);
    Botan::ECDSA_PublicKey publicKey(privateKey);
    Botan::ECDSA_PublicKey publicKey2(ecGroup,ecGroup.OS2ECP(hdKey->pub_key,EC_PUBLIC_KEY_LEN));

    unsigned char newPublicKey[EC_PUBLIC_KEY_LEN];
    wally_ec_public_key_from_private_key(hdKey->priv_key+1, EC_PRIVATE_KEY_LEN, newPublicKey, EC_PUBLIC_KEY_LEN);
    std::vector<unsigned char> publicBytes2(newPublicKey+1,newPublicKey+EC_PUBLIC_KEY_LEN);
    std::cout << "Public Key 1.1 : " << Botan::hex_encode(publicBytes2,true) << std::endl;
    std::cout << "Public Key 5.0 : " << publicKey.public_point().get_affine_x().to_hex_string() << std::endl;
    std::cout << "Public Key 6.0 : " << publicKey2.public_point().get_affine_x().to_hex_string() << std::endl;


    //Botan::PointGFp pointGFp(ecGroup.get_curve(),);
    //Botan::ECDSA_PublicKey publicKey2(ecGroup,pointGFp);*/

    //std::cout << "Private Key : " << Botan::hex_encode(hdKey->priv_key,EC_PRIVATE_KEY_LEN,true) << std::endl;

    //keto::crypto::SecureVector secureVector = Botan::PKCS8::BER_encode(privateKey);

    //std::cout << "BER Private Encoded : " << Botan::hex_encode(secureVector,true) << std::endl;

    //std::cout << "Hash : " << Botan::hex_encode(hdKey->hash160,20,true) << std::endl;

    //unsigned char serializedBytes[BIP32_SERIALIZED_LEN];
    //result = bip32_key_serialize(hdKey, BIP32_FLAG_KEY_PRIVATE, serializedBytes, BIP32_SERIALIZED_LEN);

    //if (result != WALLY_OK) {
    //    std::cout << "Failed to serialize the key" << std::endl;
    //    return -1;
    //}

    //std::cout << "HD Private key : " << Botan::hex_encode(serializedBytes,BIP32_SERIALIZED_LEN,true) << std::endl;

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
            std::cout << "Example:" << std::endl;
            std::cout << "\t./keto_cli.sh -T -a test -p 3D89018355E055923478E8E816D82A26A8AA10A2AE5B497847084AB7F54B9238 -s D594F22DC389E38B3DE7FA5630DBD9DCA16DA8A77097516FD37F9E25C6BE24D2 --target D594F22DC389E38B3DE7FA5630DBD9DCA16DA8A77097516FD37F9E25C6BE24D2 -V 100" << std::endl;
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
        } else if (config->getVariablesMap().count(keto::cli::Constants::MNEMONIC_WORD_LIST)) {
            return printWordList(config,optionDescription);
        } else if (config->getVariablesMap().count(keto::cli::Constants::GENERATE_MNEMONIC_KEY)) {
            return generateMnemonicKey(config,optionDescription);
        } else if (config->getVariablesMap().count(keto::cli::Constants::VALIDATE_SIGNATURE)) {
            return verifySignature(config,optionDescription);
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
