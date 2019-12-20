/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SandboxService.cpp
 * Author: ubuntu
 * 
 * Created on April 10, 2018, 8:33 AM
 */

#include <boost/filesystem/path.hpp>
#include <iostream>
#include <sstream>

#include <condition_variable>

#include <botan/hex.h>

#include "Sandbox.pb.h"

#include "keto/server_common/EventUtils.hpp"
#include "keto/server_common/Events.hpp"
#include "keto/server_common/EventServiceHelpers.hpp"

#include "keto/common/Log.hpp"
#include "keto/common/Exception.hpp"

#include "keto/sandbox/SandboxService.hpp"
#include "keto/wavm_common/WavmEngineManager.hpp"
#include "keto/wavm_common/WavmSessionManager.hpp"
#include "keto/wavm_common/WavmSessionScope.hpp"
#include "keto/wavm_common/WavmSessionTransaction.hpp"
#include "keto/wavm_common/WavmSessionHttp.hpp"
#include "keto/environment/EnvironmentManager.hpp"
#include "keto/server_common/VectorUtils.hpp"
#include "keto/sandbox/SandboxFork.hpp"


namespace keto {
namespace sandbox {

static SandboxServicePtr singleton;

std::string SandboxService::getSourceVersion() {
    return OBFUSCATED("$Id$");
}

SandboxService::SandboxService() {

}

SandboxService::~SandboxService() {
}


SandboxServicePtr SandboxService::init() {
    return singleton = std::make_shared<SandboxService>();
}

void SandboxService::fin() {
    singleton.reset();
}

SandboxServicePtr SandboxService::getInstance() {
    return singleton;
}

keto::event::Event SandboxService::executeActionMessage(const keto::event::Event& event) {
    return SandboxFork(event).executeActionMessage();
}

keto::event::Event SandboxService::executeHttpActionMessage(const keto::event::Event& event) {
    return SandboxFork(event).executeHttpActionMessage();
}

}
}