file(GLOB HEADERS "include/keto/event/*.hpp")

SET(EVENT_MODULE_SIGNATURE_METHOD "getEventServerModuleSignature")
SET(EVENT_MODULE_SIGNATURE "this is a signature")
SET(EVENT_MODULE_KEY_METHOD "getEventModuleKey")
SET(EVENT_MODULE_SIGNATURE "this is a key")
SET(EVENT_MODULE_SOURCE_VERSION_METHOD_MAP "getClientSourceVersionMap")
SET(EVENT_MODULE_CONSENSUS_HASH_SCRIPT_METHOD "getConsensusHashScriptConfig")

include_directories(${PROTOBUF_INCLUDE_DIR})


## SORT .cpp by most likely to change / break compile
add_library( 0001_keto_event_service_module
            SHARED

            EventServiceModuleManager.cpp
            EventServiceModule.cpp
            ConsensusService.cpp
            EventRegistry.cpp
             ${HEADERS}
           )

add_dependencies(0001_keto_event_service_module keto_tools)

KetoModuleDepencencies( 0001_keto_event_service_module 
                            PRIVATE keto_software_consensus
                            PRIVATE keto_server_common keto_asn1_common keto_asn1_protocol
                            PRIVATE keto_proto_environment ${PROTOBUF_LIBRARIES}
                            PRIVATE keto_crypto ${BOTAN_LIBRARIES} 
                            PRIVATE ${ROCKSDB_LIBRARY} ${ROCKSDB_EXTRA_LIBRARY} ${ROCKSDB_SNAPPY_LIBRARY}
                            PRIVATE ${OPENSSL_LIBRARIES})

configure_file(include/keto/event/EventServiceModuleManagerMisc.hpp.in include/keto/event/EventServiceModuleManagerMisc.hpp)

target_include_directories( 0001_keto_event_service_module
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/environment/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/module_manager/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/event_service_api/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../libs/obfuscate/include"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/libs/common/include/"
                            
                            ${Boost_INCLUDE_DIRS}
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

INSTALL( TARGETS
   0001_keto_event_service_module

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION shared
   ARCHIVE DESTINATION shared
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/event" )
