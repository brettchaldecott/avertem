file(GLOB HEADERS "include/keto/keystore/*.hpp")


SET(KEYSTORE_MODULE_SIGNATURE_METHOD "getBlockServerModuleSignature")
SET(KEYSTORE_MODULE_SIGNATURE "this is a signature")
SET(KEYSTORE_MODULE_KEY_METHOD "getBlockModuleKey")
SET(KEYSTORE_MODULE_SIGNATURE "this is a key")
SET(KEYSTORE_MODULE_SOURCE_VERSION_METHOD_MAP "getClientSourceVersionMap")
SET(KEYSTORE_MODULE_CONSENSUS_HASH_SCRIPT_METHOD "getConsensusHashScriptConfig")

# master key store
SET(KEYSTORE_MODULE_MASTER_STORE_PASSWORD_HASH "$2a$12$Venf41fgqS3l0foPb1mQgOcRPGhL8CJFmHe7cUZLe5zW9RBnWJN2W")


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
            TransactionEncryptionService.cpp
            KeyStoreReEncryptTransactionMessageHelper.cpp
            KeyStoreStorageManager.cpp
            include/keto/keystore/Constants.hpp
            KeyStoreEntry.cpp
            Constants.cpp
            NetworkSessionKeyEncryptor.cpp
            NetworkSessionKeyDecryptor.cpp
            NetworkSessionKeyManager.cpp
            NetworkSessionKeyTransactionEncryptionHandler.cpp
            KeyStoreWrapIndexManager.cpp
            KeyStoreWrapIndexDecryptor.cpp
            KeyStoreWrapIndexEncryptor.cpp
            KeyStoreWrapEntry.cpp
            MasterKeyManager.cpp
            EncryptionService.cpp

             ${HEADERS} )

add_dependencies(0010_keto_keystore_module avertem_tools)

KetoModuleDepencencies( 0010_keto_keystore_module 
                            PRIVATE keto_memory_vault_session keto_software_consensus keto_key_store_db keto_memory_vault
                            PRIVATE keto_transaction_common keto_server_common keto_asn1_common keto_asn1_protocol
                            PRIVATE keto_key_tools keto_rpc_protocol keto_proto_environment keto_rocks_db ${PROTOBUF_LIBRARIES}
                            PRIVATE keto_crypto key_store_utils
                            PRIVATE ${ROCKSDB_LIBRARIES}
                            PRIVATE ${BOTAN_LIBRARIES}
                            PRIVATE ${OPENSSL_LIBRARIES} )

KetoModuleConsensusScriptsCalls( 0010_keto_keystore_module ${NUMBER_OF_KEYS} "4"
        PRIVATE keto_memory_vault_session keto_software_consensus keto_key_store_db keto_memory_vault
        PRIVATE keto_transaction_common keto_server_common keto_asn1_common keto_asn1_protocol
        PRIVATE keto_key_tools keto_rpc_protocol keto_proto_environment keto_rocks_db ${PROTOBUF_LIBRARIES}
        PRIVATE keto_crypto key_store_utils
        PRIVATE ${ROCKSDB_LIBRARIES}
        PRIVATE ${BOTAN_LIBRARIES}
        PRIVATE ${OPENSSL_LIBRARIES} )
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

KetoModuleConsensus( 0010_keto_keystore_module ${NUMBER_OF_KEYS} keystore )
configure_file(include/keto/keystore/KeystoreModuleManagerMisc.hpp.in include/keto/keystore/KeystoreModuleManagerMisc.hpp)
configure_file(include/keto/keystore/MasterKeyManagerMisc.hpp.in include/keto/keystore/MasterKeyManagerMisc.hpp)


target_include_directories( 0010_keto_keystore_module
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
   0010_keto_keystore_module

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION shared
   ARCHIVE DESTINATION shared
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/keystore" )
