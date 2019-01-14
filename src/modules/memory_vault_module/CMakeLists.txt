file(GLOB HEADERS "include/keto/memory_vault_module/*.hpp")


SET(MEMORY_VAULT_MODULE_SIGNATURE_METHOD "getMemoryVaultServerModuleSignature")
SET(MEMORY_VAULT_MODULE_SIGNATURE "this is a signature")
SET(MEMORY_VAULT_MODULE_KEY_METHOD "getMemoryVaultModuleKey")
SET(MEMORY_VAULT_MODULE_SIGNATURE "this is a key")
SET(MEMORY_VAULT_MODULE_SOURCE_VERSION_METHOD_MAP "getClientSourceVersionMap")
SET(MEMORY_VAULT_MODULE_CONSENSUS_HASH_SCRIPT_METHOD "getConsensusHashScriptConfig")

include_directories(${PROTOBUF_INCLUDE_DIR})

## SORT .cpp by most likely to change / break compile
add_library( 0009_keto_memory_vault_module
            SHARED

            MemoryVaultModule.cpp
            MemoryVaultModuleManager.cpp
            ConsensusService.cpp
            EventRegistry.cpp

             ${HEADERS} )

add_dependencies(0009_keto_memory_vault_module keto_tools)

KetoModuleDepencencies( 0009_keto_memory_vault_module
                            PRIVATE keto_software_consensus keto_memory_vault
                            PRIVATE keto_transaction_common keto_server_common keto_asn1_common keto_asn1_protocol
                            PRIVATE keto_key_tools keto_proto_environment keto_rocks_db ${PROTOBUF_LIBRARIES}
                            PRIVATE keto_crypto key_store_utils
                            PRIVATE ${ROCKSDB_LIBRARIES}
                            PRIVATE ${BOTAN_LIBRARIES}
                            PRIVATE ${OPENSSL_LIBRARIES} )

KetoModuleConsensus( 0009_keto_memory_vault_module ${NUMBER_OF_KEYS} "memory_vault" )

configure_file(include/keto/memory_vault_module/MemoryVaultModuleManagerMisc.hpp.in include/keto/memory_vault_module/MemoryVaultModuleManagerMisc.hpp)

target_include_directories( 0009_keto_memory_vault_module
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/environment/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/obfuscate/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/module_manager/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/event_service_api/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/server_common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/key_store_utils/include"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/libs/common/include/"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/protocol/proto/"
                            PUBLIC "${PROTO_HDRS}"

                            ${Boost_INCLUDE_DIRS}
                            ${BOTAN_INCLUDE_DIR}
                            ${JSON_INCLUDE_DIR}
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

INSTALL( TARGETS
   0009_keto_memory_vault_module

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION shared
   ARCHIVE DESTINATION shared
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/memory_vault_module" )
