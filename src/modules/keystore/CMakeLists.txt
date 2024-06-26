file(GLOB HEADERS "include/keto/keystore/*.hpp")


SET(KEYSTORE_MODULE_SIGNATURE_METHOD "getBlockServerModuleSignature")
SET(KEYSTORE_MODULE_SIGNATURE "this is a signature")
SET(KEYSTORE_MODULE_KEY_METHOD "getBlockModuleKey")
SET(KEYSTORE_MODULE_SIGNATURE "this is a key")
SET(KEYSTORE_MODULE_SOURCE_VERSION_METHOD_MAP "getClientSourceVersionMap")
SET(KEYSTORE_MODULE_CONSENSUS_HASH_SCRIPT_METHOD "getConsensusHashScriptConfig")

include_directories(${PROTOBUF_INCLUDE_DIR})

## SORT .cpp by most likely to change / break compile
add_library( 0010_keto_keystore_module
            SHARED

            KeystoreModuleManager.cpp
            KeystoreModule.cpp
            EventRegistry.cpp
            KeyStoreService.cpp
            SessionKeyManager.cpp
            ConsensusService.cpp

             ${HEADERS}
           )

add_dependencies(0010_keto_keystore_module keto_tools)

KetoModuleDepencencies( 0010_keto_keystore_module 
                            PRIVATE keto_software_consensus
                            PRIVATE keto_server_common keto_asn1_common keto_asn1_protocol
                            PRIVATE keto_key_tools keto_proto_environment ${PROTOBUF_LIBRARIES}
                            PRIVATE keto_crypto ${BOTAN_LIBRARIES} 
                            PRIVATE ${OPENSSL_LIBRARIES} )

KetoModuleConsensus( 0010_keto_keystore_module ${NUMBER_OF_KEYS} keystore )

configure_file(include/keto/keystore/KeystoreModuleManagerMisc.hpp.in include/keto/keystore/KeystoreModuleManagerMisc.hpp)

target_include_directories( 0010_keto_keystore_module
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/environment/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/obfuscate/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/module_manager/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/event_service_api/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/server_common/include"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/libs/common/include/"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/protocol/proto/"
                            PUBLIC "${PROTO_HDRS}"

                            ${Boost_INCLUDE_DIRS}
                            ${BOTAN_INCLUDE_DIR}
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

INSTALL( TARGETS
   0010_keto_keystore_module

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION shared
   ARCHIVE DESTINATION shared
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/keystore" )
