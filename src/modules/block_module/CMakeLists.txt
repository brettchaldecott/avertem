file(GLOB HEADERS "include/keto/block/*.hpp")

SET(BLOCK_MODULE_SIGNATURE_METHOD "getBlockServerModuleSignature")
SET(BLOCK_MODULE_SIGNATURE "this is a signature")
SET(BLOCK_MODULE_KEY_METHOD "getBlockModuleKey")
SET(BLOCK_MODULE_SIGNATURE "this is a key")
SET(BLOCK_MODULE_SOURCE_VERSION_METHOD_MAP "getClientSourceVersionMap")
SET(BLOCK_MODULE_CONSENSUS_HASH_SCRIPT_METHOD "getConsensusHashScriptConfig")

include_directories(${PROTOBUF_INCLUDE_DIR})

## SORT .cpp by most likely to change / break compile
add_library( 0020_keto_block_module
            SHARED

            BlockModuleManager.cpp
            BlockModule.cpp
            StorageManager.cpp
            BlockService.cpp
            EventRegistry.cpp
            Constants.cpp
            GenesisReader.cpp
            GenesisLoader.cpp
            BlockProducer.cpp
            TransactionProcessor.cpp
            ConsensusService.cpp

             ${HEADERS}
           )

add_dependencies(0020_keto_block_module keto_tools)

KetoModuleDepencencies( 0020_keto_block_module
                            PRIVATE keto_software_consensus
                            PRIVATE keto_block_db keto_chain_common keto_rocks_db
                            PRIVATE keto_key_tools keto_server_common keto_asn1_common 
                            PRIVATE keto_transaction_common keto_proto_environment 
                            PRIVATE keto_asn1_protocol ${PROTOBUF_LIBRARIES}
                            PRIVATE keto_crypto ${BOTAN_LIBRARIES} 
                            PRIVATE ${ROCKSDB_LIBRARIES}
                            PRIVATE ${OPENSSL_LIBRARIES} )

KetoModuleConsensus( 0020_keto_block_module ${NUMBER_OF_KEYS} block )

configure_file(include/keto/block/BlockModuleManagerMisc.hpp.in include/keto/block/BlockModuleManagerMisc.hpp)

target_include_directories( 0020_keto_block_module
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/obfuscate/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/environment/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/module_manager/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/block_db/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/chain_common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/event_service_api/include"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/libs/common/include/"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/libs/asn1/include/"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/protocol/proto/"
                            PUBLIC "${PROTO_HDRS}"

                            ${Boost_INCLUDE_DIRS}
                            ${JSON_INCLUDE_DIR}
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

INSTALL( TARGETS
   0020_keto_block_module

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION shared
   ARCHIVE DESTINATION shared
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/block" )
