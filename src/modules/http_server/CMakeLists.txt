file(GLOB HEADERS "include/keto/http/*.hpp")


SET(HTTPD_MODULE_SIGNATURE_METHOD "getHttpServerModuleSignature")
SET(HTTPD_MODULE_SIGNATURE "this is a signature")
SET(HTTPD_MODULE_KEY_METHOD "getBlockModuleKey")
SET(HTTPD_MODULE_SIGNATURE "this is a key")
SET(HTTPD_MODULE_SOURCE_VERSION_METHOD_MAP "getClientSourceVersionMap")
SET(HTTPD_MODULE_CONSENSUS_HASH_SCRIPT_METHOD "getConsensusHashScriptConfig")

include_directories(${PROTOBUF_INCLUDE_DIR})

## SORT .cpp by most likely to change / break compile
add_library( 0050_keto_httpd_module
            SHARED

            HttpdModuleManager.cpp
            HttpdModule.cpp
            HttpdServer.cpp
            ConsensusService.cpp
            EventRegistry.cpp

             ${HEADERS}
           )

add_dependencies(0050_keto_httpd_module keto_tools)

KetoModuleDepencencies( 0050_keto_httpd_module 
                            PRIVATE keto_http_server_session
                            PRIVATE keto_software_consensus
                            PRIVATE keto_transaction_common keto_server_common
                            PRIVATE keto_key_tools keto_proto_environment ${PROTOBUF_LIBRARIES}
                            PRIVATE keto_asn1_common keto_asn1_protocol
                            PRIVATE keto_crypto ${BOTAN_LIBRARIES} 
                            PRIVATE ${OPENSSL_LIBRARIES}
 )

KetoModuleConsensus( 0050_keto_httpd_module ${NUMBER_OF_KEYS} http )

configure_file(include/keto/http/HttpdModuleManagerMisc.hpp.in include/keto/http/HttpdModuleManagerMisc.hpp)

target_include_directories( 0050_keto_httpd_module
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/environment/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/obfuscate/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/module_manager/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/event_service_api/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/transaction_manager_api/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/http_server_session/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/ssl/include"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/libs/common/include/"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/protocol/proto/"
                            PUBLIC "${PROTO_HDRS}"

                            ${Boost_INCLUDE_DIRS}
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

INSTALL( TARGETS
   0050_keto_httpd_module

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION shared
   ARCHIVE DESTINATION shared
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/http" )
