file(GLOB HEADERS "include/keto/key_tools/*.hpp")

include_directories(${PROTOBUF_INCLUDE_DIR})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

## SORT .cpp by most likely to change / break compile
add_library( keto_key_tools
            
            ContentDecryptor.cpp
            ContentEncryptor.cpp
            ContentSigner.cpp
            KeyPairCreator.cpp
            KeyUtils.cpp

             ${HEADERS}
           )

target_link_libraries( keto_key_tools )
target_include_directories( keto_key_tools
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../obfuscate/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../environment/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../event_service_api/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../transaction_manager_api/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../module_manager/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../asn1/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../server_common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../crypto/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../obfuscate/include"
                            
                            PUBLIC "${CMAKE_BINARY_DIR}/src/protocol/proto/"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/protocol/asn1/" 
                            PUBLIC "${CMAKE_BINARY_DIR}/src/libs/common/include/" 
                            PUBLIC "${PROTO_HDRS}"

                            ${Boost_INCLUDE_DIRS}
                            ${BOTAN_INCLUDE_DIR}
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

INSTALL( TARGETS
   keto_key_tools

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/key_tools" )
