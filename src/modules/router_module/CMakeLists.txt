file(GLOB HEADERS "include/keto/router/*.hpp")

SET(ROUTER_MODULE_SIGNATURE_METHOD "getRouterServerModuleSignature")
SET(ROUTER_MODULE_SIGNATURE "this is a signature")
SET(ROUTER_MODULE_KEY_METHOD "getRouterModuleKey")
SET(ROUTER_MODULE_SIGNATURE "this is a key")
SET(ROUTER_MODULE_SOURCE_VERSION_METHOD_MAP "getClientSourceVersionMap")
SET(ROUTER_MODULE_CONSENSUS_HASH_SCRIPT_METHOD "getConsensusHashScriptConfig")

include_directories(${PROTOBUF_INCLUDE_DIR})

## SORT .cpp by most likely to change / break compile
add_library( 0015_keto_router_module
            SHARED

            RouterModuleManager.cpp
            RouterModule.cpp
            RouterService.cpp
            EventRegistry.cpp
            ConsensusService.cpp
            StorageManager.cpp
            RouterRegistry.cpp
            PeerCache.cpp

             ${HEADERS}
           )

add_dependencies(0015_keto_router_module keto_tools)

KetoModuleDepencencies( 0015_keto_router_module 
                            PRIVATE keto_software_consensus
                            PRIVATE keto_router_db keto_rocks_db
                            PRIVATE keto_server_common keto_transaction_common keto_router_utils keto_router_utils
                            PRIVATE keto_key_tools keto_proto_environment ${PROTOBUF_LIBRARIES}
                            PRIVATE keto_asn1_common keto_asn1_protocol
                            PRIVATE keto_crypto ${BOTAN_LIBRARIES} 
                            PRIVATE ${ROCKSDB_LIBRARY}
                            PRIVATE ${OPENSSL_LIBRARIES} )

KetoModuleConsensus( 0015_keto_router_module ${NUMBER_OF_KEYS} router )

configure_file(include/keto/router/RouterModuleManagerMisc.hpp.in include/keto/router/RouterModuleManagerMisc.hpp)

target_include_directories( 0015_keto_router_module
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/environment/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/module_manager/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/obfuscate/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/event_service_api/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/transaction_common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/server_common/include"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/libs/common/include/"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/protocol/proto/"
                            PUBLIC "${PROTO_HDRS}"
                            
                            ${Boost_INCLUDE_DIRS}
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

INSTALL( TARGETS
   0015_keto_router_module

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION shared
   ARCHIVE DESTINATION shared
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/router" )
