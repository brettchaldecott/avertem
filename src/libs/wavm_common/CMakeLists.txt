file(GLOB HEADERS "include/keto/wavm_common/*.hpp")

include_directories(${PROTOBUF_INCLUDE_DIR})

## SORT .cpp by most likely to change / break compile
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set( CMAKE_VERBOSE_MAKEFILE on )

if(MSVC)
    add_definitions("-DDLL_EXPORT=__declspec(dllexport)")
    add_definitions("-DDLL_IMPORT=__declspec(dllimport)")
else()
    add_definitions(-DDLL_EXPORT=)
    add_definitions(-DDLL_IMPORT=)
endif()

add_library( keto_wavm_common
            
            WavmEngineManager.cpp
            WavmEngineWrapper.cpp
            Emscripten.cpp
            WavmUtils.cpp
            RDFMemorySession.cpp
            WavmSession.cpp
            WavmSessionTransaction.cpp
            WavmSessionTransactionBuilder.cpp
            WavmSessionHttp.cpp
            WavmSessionManager.cpp
            WavmSessionScope.cpp
            RDFURLUtils.cpp
            RDFConstants.cpp
            Constants.cpp
            
             ${HEADERS}
           )

target_link_libraries( keto_wavm_common
                    ${LIBWAVM_LIBRARY}
                    ${LIBWAVM_WAVMUnwind_LIBRARY}
                    ${LIBWAVM_WAVMlmdb_LIBRARY}
                    ${LIBWAVM_PLATFORM_LIBRARY}
                    ${LIBWAVM_LLVM_LIBS}
        )
target_include_directories( keto_wavm_common
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../common/include"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/protocol/asn1/" 
                            PUBLIC "${CMAKE_BINARY_DIR}/src/protocol/proto/" 
                            PUBLIC "${CMAKE_BINARY_DIR}/src/libs/common/include/"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../obfuscate/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../asn1/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../environment/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../crypto/include/"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../server_common/include/"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../transaction_common/include/"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../event_service_api/include/"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../module_manager/include/"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../account_query/include/"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../chain_query_common/include/"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../chain_common/include/"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../key_store_utils/include/"
                            PUBLIC "${PROTO_HDRS}"
                            ${Boost_INCLUDE_DIRS}
                            ${BOTAN_INCLUDE_DIR}
                            ${LIBWAVM_INCLUDE_DIR}
                            ${LIBWAVM_LLVM_INCLUDE_DIRS}
                            ${LIBRDF_INCLUDE_DIR}
                            ${LIBRDF_RAPTOR_INCLUDE_DIR}
                            ${LIBRDF_RASQAL_INCLUDE_DIR}
                            )

target_compile_definitions(keto_wavm_common PUBLIC "WAVM_API=")

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

INSTALL( TARGETS
   keto_wavm_common

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/wavm_common" )
