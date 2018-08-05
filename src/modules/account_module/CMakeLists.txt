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

add_dependencies(0015_account_module keto_tools)


KetoModuleDepencencies( 0015_account_module 
                            PRIVATE keto_account_db keto_rocks_db keto_transaction_common 
                            PRIVATE keto_software_consensus
                            PRIVATE keto_server_common keto_asn1_common keto_asn1_protocol
                            PRIVATE keto_proto_environment ${PROTOBUF_LIBRARIES}
                            PRIVATE keto_crypto ${BOTAN_LIBRARIES} 
                            PRIVATE ${ROCKSDB_LIBRARY} ${ROCKSDB_EXTRA_LIBRARY} ${ROCKSDB_SNAPPY_LIBRARY}
                            PRIVATE ${LIBRDF_LIBRARY} ${LIBRDF_RAPTOR_LIBRARY} 
                            PRIVATE ${LIBRDF_RASQAL_LIBRARY} 
                            PRIVATE ${LIBRDF_EXTRA_LIBRARY} ${LIBRDF_XML_EXTRA_LIBRARY} 
                            PRIVATE ${LIBRDF_PCRE_EXTRA_LIBRARY} ${LIBRDF_LTDL_EXTRA_LIBRARY}
                            PRIVATE ${LIBRDF_GMP_EXTRA_LIBRARY}
                            PRIVATE ${OPENSSL_LIBRARIES} )

configure_file(include/keto/account/AccountModuleManagerMisc.hpp.in include/keto/account/AccountModuleManagerMisc.hpp)

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
