file(GLOB HEADERS "include/keto/consensus_module/*.hpp")


SET(CONSENSUS_MODULE_SIGNATURE_METHOD "getConsensusModuleSignature")
SET(CONSENSUS_MODULE_SIGNATURE "this is a signature")
SET(CONSENSUS_MODULE_KEY_METHOD "getConsensusModuleKey")
SET(CONSENSUS_MODULE_SIGNATURE "this is a key")
SET(CONSENSUS_SOURCE_VERSION_METHOD_MAP "getClientSourceVersionMap")
SET(CONSENSUS_CONSENSUS_HASH_SCRIPT_METHOD "getConsensusHashScriptConfig")

include_directories(${PROTOBUF_INCLUDE_DIR})

## SORT .cpp by most likely to change / break compile
add_library( 0100_keto_consensus_module
            SHARED

            ConsensusModuleManager.cpp
            ConsensusModule.cpp
            ConsensusServices.cpp
            EventRegistry.cpp
            ConsensusServer.cpp

             ${HEADERS}
           )

add_dependencies(0100_keto_consensus_module keto_tools)

KetoModuleDepencencies( 0100_keto_consensus_module 
                            PRIVATE keto_software_consensus
                            PRIVATE keto_transaction_common keto_server_common
                            PRIVATE keto_key_tools keto_proto_environment ${PROTOBUF_LIBRARIES}
                            PRIVATE keto_asn1_common keto_asn1_protocol
                            PRIVATE keto_crypto ${BOTAN_LIBRARIES} 
                            PRIVATE ${OPENSSL_LIBRARIES} )

KetoModuleConsensus( 0100_keto_consensus_module ${NUMBER_OF_KEYS} consensus_module )

configure_file(include/keto/consensus_module/ConsensusModuleManagerMisc.hpp.in include/keto/consensus_module/ConsensusModuleManagerMisc.hpp)

target_include_directories( 0100_keto_consensus_module
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/environment/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/obfuscate/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/module_manager/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/event_service_api/include"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/libs/common/include/"
                            PUBLIC "${PROTO_HDRS}"

                            ${Boost_INCLUDE_DIRS}
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

INSTALL( TARGETS
   0100_keto_consensus_module

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION shared
   ARCHIVE DESTINATION shared
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/consensus_module" )
