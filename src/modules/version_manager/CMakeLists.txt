file(GLOB HEADERS "include/keto/version_manager/*.hpp")


SET(VERSION_MANAGER_MODULE_SIGNATURE_METHOD "getTransactionModuleSignature")
SET(VERSION_MANAGER_MODULE_SIGNATURE "this is a signature")
SET(VERSION_MANAGER_MODULE_KEY_METHOD "getTransactionModuleKey")
SET(VERSION_MANAGER_MODULE_SIGNATURE "this is a key")
SET(VERSION_MANAGER_MODULE_SOURCE_VERSION_METHOD_MAP "getClientSourceVersionMap")
SET(VERSION_MANAGER_MODULE_CONSENSUS_HASH_SCRIPT_METHOD "getConsensusHashScriptConfig")

include_directories(${PROTOBUF_INCLUDE_DIR})


## SORT .cpp by most likely to change / break compile
add_library( 0070_keto_version_manager_module
            SHARED

            VersionManagerModuleManager.cpp
            VersionManagerModule.cpp
            ConsensusService.cpp
            EventRegistry.cpp

             ${HEADERS}
           )

add_dependencies(0070_keto_version_manager_module keto_tools)


KetoModuleDepencencies( 0070_keto_version_manager_module 
                            PRIVATE keto_software_consensus
                            PRIVATE keto_transaction_common keto_server_common keto_rpc_protocol
                            PRIVATE keto_proto_environment ${PROTOBUF_LIBRARIES}
                            PRIVATE keto_asn1_common keto_asn1_protocol
                            PRIVATE keto_crypto ${BOTAN_LIBRARIES} 
                            PRIVATE ${OPENSSL_LIBRARIES} )

KetoModuleConsensus( 0070_keto_version_manager_module ${NUMBER_OF_KEYS} "version_manager" )

configure_file(include/keto/version_manager/VersionManagerModuleManagerMisc.hpp.in include/keto/version_manager/VersionManagerModuleManagerMisc.hpp)

target_include_directories( 0070_keto_version_manager_module
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/environment/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/obfuscate/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/module_manager/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/event_service_api/include"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/libs/common/include/"

                            ${Boost_INCLUDE_DIRS}
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

INSTALL( TARGETS
   0070_keto_version_manager_module

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION shared
   ARCHIVE DESTINATION shared
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/version_manager" )
