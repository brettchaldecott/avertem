/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   KeyLoader.cpp
 * Author: ubuntu
 * 
 * Created on February 14, 2018, 7:04 AM
 */

#include <boost/filesystem/path.hpp>

#include <botan/pkcs8.h>
#include <botan/hash.h>
#include <botan/data_src.h>
#include <botan/pubkey.h>
#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/hex.h>


#include "keto/server_common/StringUtils.hpp"
#include "keto/crypto/Exception.hpp"
#include "keto/crypto/KeyLoader.hpp"
#include "keto/environment/EnvironmentManager.hpp"

namespace keto {
namespace crypto {

std::string KeyLoader::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

KeyLoader::KeyLoader(const boost::filesystem::path& publicKeyPath) : initialized(true),
        publicKeyPath(publicKeyPath.string()), generator(new Botan::AutoSeeded_RNG())
{

}

KeyLoader::KeyLoader(const std::string& publicKeyPath) : initialized(true),
        publicKeyPath(publicKeyPath), generator(new Botan::AutoSeeded_RNG())
{
    boost::filesystem::path publicPath =
            keto::environment::EnvironmentManager::getInstance()->getEnv()->getInstallDir() / publicKeyPath;

    if (boost::filesystem::exists(publicPath)) {
        // setup the paths using the environmental variables
        boost::filesystem::path publicPath =
                keto::environment::EnvironmentManager::getInstance()->getEnv()->getInstallDir() / publicKeyPath;

        if (!boost::filesystem::exists(publicPath)) {
            BOOST_THROW_EXCEPTION(keto::crypto::InvalidKeyPathException());
        }
        this->publicKeyPath = publicPath.string();
    }
}
    
    
KeyLoader::KeyLoader(const std::string& privateKeyPath, 
        const std::string& publicKeyPath) : initialized(true), 
        privateKeyPath(privateKeyPath), publicKeyPath(publicKeyPath),
        generator(new Botan::AutoSeeded_RNG())
{
    
    // setup the paths using the environmental variables
    if (!keto::server_common::StringUtils::isHexidecimal(publicKeyPath)) {
        boost::filesystem::path publicPath =
                keto::environment::EnvironmentManager::getInstance()->getEnv()->getInstallDir() / publicKeyPath;
        if (!boost::filesystem::exists(publicPath)) {
            BOOST_THROW_EXCEPTION(keto::crypto::InvalidKeyPathException());
        }
        this->publicKeyPath = publicPath.string();
    }
    
    // setup the paths using the environmental variables
    if (!keto::server_common::StringUtils::isHexidecimal(privateKeyPath)) {
        boost::filesystem::path privatePath =
                keto::environment::EnvironmentManager::getInstance()->getEnv()->getInstallDir() / privateKeyPath;
        if (!boost::filesystem::exists(privatePath)) {
            BOOST_THROW_EXCEPTION(keto::crypto::InvalidKeyPathException());
        }
        this->privateKeyPath = privatePath.string();
    }
}

KeyLoader::~KeyLoader() {
}

std::shared_ptr<Botan::Private_Key> KeyLoader::getPrivateKey() {
    if (!isInitialized()) {
        BOOST_THROW_EXCEPTION(keto::crypto::KeyLoaderNotInitializedException());
    }
    if (this->privateKeyPath.empty()) {
        BOOST_THROW_EXCEPTION(keto::crypto::PrivateKeyNotConfiguredException());
    }
    // attempt to load the private key using the path supplied and the
    if (!keto::server_common::StringUtils::isHexidecimal(this->privateKeyPath)) {
        KETO_LOG_DEBUG << "Load the file [" << this->privateKeyPath << "]";
        return std::shared_ptr<Botan::Private_Key>(
                Botan::PKCS8::load_key(this->privateKeyPath, *generator));

    } else {
        KETO_LOG_DEBUG << "Load the hex encoded entry [" << this->privateKeyPath << "]";
        Botan::DataSource_Memory derivedDatasource(Botan::hex_decode(this->privateKeyPath,true));
        return std::shared_ptr<Botan::Private_Key>(
                Botan::PKCS8::load_key(derivedDatasource));
    }
}


std::shared_ptr<Botan::Public_Key> KeyLoader::getPublicKey() {
    if (!isInitialized()) {
        BOOST_THROW_EXCEPTION(keto::crypto::KeyLoaderNotInitializedException());
    }
    if (this->publicKeyPath.empty()) {
        BOOST_THROW_EXCEPTION(keto::crypto::PublicKeyNotConfiguredException());
    }
    // attempt to load the private key using the path supplied and the
    if (!keto::server_common::StringUtils::isHexidecimal(this->publicKeyPath)) {
        KETO_LOG_DEBUG << "Load the public key from file [" << this->publicKeyPath << "]";
        return std::shared_ptr<Botan::Public_Key>(
                Botan::X509::load_key(this->publicKeyPath));
    } else {
        KETO_LOG_DEBUG << "Load the public key from hex [" << this->publicKeyPath << "]";
        return std::shared_ptr<Botan::Public_Key>(
                Botan::X509::load_key(Botan::hex_decode(this->publicKeyPath,true)));
    }

}

bool KeyLoader::isInitialized() {
    return this->initialized;
}


}
}
