//
// Created by Brett Chaldecott on 2021/10/23.
//

#include "keto/block/ElectionTangleManager.hpp"

namespace keto {
namespace block {

static ElectionTangleManagerPtr singleton;

std::string ElectionTangleManager::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

ElectionTangleManagerPtr ElectionTangleManager::init() {
    if (singleton) {
        return singleton;
    }

}

ElectionTangleManagerPtr ElectionTangleManager::getInstance() {

}

ElectionTangleManagerPtr ElectionTangleManager::fin() {

}

}
}