file(GLOB HEADERS "include/keto/rpc_server/*.hpp")


SET(RPC_SERVER_MODULE_SIGNATURE_METHOD "getRpcServerModuleSignature")
SET(RPC_SERVER_MODULE_SIGNATURE "this is a signature")
SET(RPC_SERVER_MODULE_KEY_METHOD "getRpcClientModuleKey")
SET(RPC_SERVER_MODULE_SIGNATURE "this is a key")
SET(RPC_SERVER_SOURCE_VERSION_METHOD_MAP "getClientSourceVersionMap")
SET(RPC_SERVER_CONSENSUS_HASH_SCRIPT_METHOD "getConsensusHashScriptConfig")

find_package(OpenSSL REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})


## SORT .cpp by most likely to change / break compile
add_library( 0030_keto_rpc_server_module
            SHARED

            RpcServerModuleManager.cpp
            RpcServerModule.cpp
            RpcServer.cpp
            EventRegistry.cpp
            RpcServerService.cpp
            ConsensusService.cpp

             ${HEADERS}
           )

add_dependencies(0030_keto_rpc_server_module keto_tools)

KetoModuleDepencencies( 0030_keto_rpc_server_module 
                            PRIVATE keto_software_consensus
                            PRIVATE keto_transaction_common keto_server_common
                            PRIVATE keto_proto_environment ${PROTOBUF_LIBRARIES}
                            PRIVATE keto_asn1_common keto_asn1_protocol
                            PRIVATE keto_crypto ${BOTAN_LIBRARIES} 
                            PRIVATE ${OPENSSL_LIBRARIES} )

configure_file(include/keto/rpc_server/RpcServerModuleManagerMisc.hpp.in include/keto/rpc_server/RpcServerModuleManagerMisc.hpp)

target_include_directories( 0030_keto_rpc_server_module
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/environment/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/module_manager/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/obfuscate/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/event_service_api/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/server_common/include"
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
   0030_keto_rpc_server_module

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION shared
   ARCHIVE DESTINATION shared
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/rpc_server" )
