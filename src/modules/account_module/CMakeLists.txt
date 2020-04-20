file(GLOB HEADERS "include/keto/account/*.hpp")


SET(ACCOUNT_MODULE_SIGNATURE_METHOD "getAccountServerModuleSignature")
SET(ACCOUNT_MODULE_SIGNATURE "this is a signature")
SET(ACCOUNT_MODULE_KEY_METHOD "getAccountModuleKey")
SET(ACCOUNT_MODULE_SIGNATURE "this is a key")
SET(ACCOUNT_MODULE_SOURCE_VERSION_METHOD_MAP "getClientSourceVersionMap")
SET(ACCOUNT_MODULE_CONSENSUS_HASH_SCRIPT_METHOD "getConsensusHashScriptConfig")



include_directories(${PROTOBUF_INCLUDE_DIR})

## SORT .cpp by most likely to change / break compile
add_library( 0015_account_module
            SHARED

            StorageManager.cpp
            AccountModuleManager.cpp
            AccountModule.cpp
            AccountService.cpp
            ConsensusService.cpp
            EventRegistry.cpp

             ${HEADERS}
           )

add_dependencies(0015_account_module avertem_tools)


KetoModuleDepencencies( 0015_account_module 
                            PRIVATE keto_account_db keto_rocks_db keto_transaction_common 
                            PRIVATE keto_software_consensus keto_rdf_utils
                            PRIVATE keto_server_common keto_asn1_common keto_asn1_protocol
                            PRIVATE keto_key_tools keto_proto_environment ${PROTOBUF_LIBRARIES}
                            PRIVATE keto_crypto ${BOTAN_LIBRARIES} 
                            PRIVATE ${ROCKSDB_LIBRARIES}
                            PRIVATE ${LIBRDF_LIBRARY} ${LIBRDF_RAPTOR_LIBRARY} 
                            PRIVATE ${LIBRDF_RASQAL_LIBRARY} 
                            PRIVATE ${LIBRDF_EXTRA_LIBRARY} ${LIBRDF_XML_EXTRA_LIBRARY} 
                            PRIVATE ${LIBRDF_PCRE_EXTRA_LIBRARY} ${LIBRDF_LTDL_EXTRA_LIBRARY}
                            PRIVATE ${LIBRDF_GMP_EXTRA_LIBRARY}
                            PRIVATE ${OPENSSL_LIBRARIES} )

KetoModuleConsensus( 0015_account_module ${NUMBER_OF_KEYS} account )

KetoModuleConsensusScriptsCalls( 0015_account_module ${NUMBER_OF_KEYS} "4"
        PRIVATE keto_account_db keto_rocks_db keto_transaction_common
        PRIVATE keto_software_consensus keto_rdf_utils
        PRIVATE keto_server_common keto_asn1_common keto_asn1_protocol
        PRIVATE keto_key_tools keto_proto_environment ${PROTOBUF_LIBRARIES}
        PRIVATE keto_crypto ${BOTAN_LIBRARIES}
        PRIVATE ${ROCKSDB_LIBRARIES}
        PRIVATE ${LIBRDF_LIBRARY} ${LIBRDF_RAPTOR_LIBRARY}
        PRIVATE ${LIBRDF_RASQAL_LIBRARY}
        PRIVATE ${LIBRDF_EXTRA_LIBRARY} ${LIBRDF_XML_EXTRA_LIBRARY}
        PRIVATE ${LIBRDF_PCRE_EXTRA_LIBRARY} ${LIBRDF_LTDL_EXTRA_LIBRARY}
        PRIVATE ${LIBRDF_GMP_EXTRA_LIBRARY}
        PRIVATE ${OPENSSL_LIBRARIES} )

configure_file(include/keto/account/AccountModuleManagerMisc.hpp.in include/keto/account/AccountModuleManagerMisc.hpp)
configure_file(consensus/consensus_1.chai.in consensus/consensus_1.chai)
configure_file(consensus/consensus_2.chai.in consensus/consensus_2.chai)
configure_file(consensus/consensus_3.chai.in consensus/consensus_3.chai)
configure_file(consensus/consensus_4.chai.in consensus/consensus_4.chai)
configure_file(consensus/consensus_5.chai.in consensus/consensus_5.chai)
configure_file(consensus/consensus_6.chai.in consensus/consensus_6.chai)
configure_file(consensus/consensus_7.chai.in consensus/consensus_7.chai)
configure_file(consensus/consensus_8.chai.in consensus/consensus_8.chai)
configure_file(consensus/consensus_9.chai.in consensus/consensus_9.chai)
configure_file(consensus/consensus_10.chai.in consensus/consensus_10.chai)
configure_file(consensus/consensus_11.chai.in consensus/consensus_11.chai)
configure_file(consensus/consensus_12.chai.in consensus/consensus_12.chai)
configure_file(consensus/consensus_13.chai.in consensus/consensus_13.chai)
configure_file(consensus/consensus_14.chai.in consensus/consensus_14.chai)
configure_file(consensus/consensus_15.chai.in consensus/consensus_15.chai)
configure_file(consensus/consensus_16.chai.in consensus/consensus_16.chai)
configure_file(consensus/consensus_17.chai.in consensus/consensus_17.chai)
configure_file(consensus/consensus_18.chai.in consensus/consensus_18.chai)
configure_file(consensus/consensus_19.chai.in consensus/consensus_19.chai)
configure_file(consensus/consensus_20.chai.in consensus/consensus_20.chai)
configure_file(consensus/consensus_21.chai.in consensus/consensus_21.chai)
configure_file(consensus/consensus_22.chai.in consensus/consensus_22.chai)
configure_file(consensus/consensus_23.chai.in consensus/consensus_23.chai)
configure_file(consensus/consensus_24.chai.in consensus/consensus_24.chai)

target_include_directories( 0015_account_module
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/environment/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/obfuscate/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/module_manager/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/event_service_api/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/account_db/include"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/libs/common/include/"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/protocol/proto/"
                            PUBLIC "${PROTO_HDRS}"

                            ${Boost_INCLUDE_DIRS}
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

INSTALL( TARGETS
   0015_account_module

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION shared
   ARCHIVE DESTINATION shared
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/account" )
