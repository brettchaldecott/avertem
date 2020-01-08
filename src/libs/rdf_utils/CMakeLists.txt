file(GLOB HEADERS "include/keto/rdf_utils/*.hpp")

set(HAVE_CONFIG_H DEFINE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")

add_library( keto_rdf_utils

        Constants.cpp
        RDFQueryParser.cpp

           )

target_link_libraries( keto_rdf_utils )
target_include_directories( keto_rdf_utils
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../obfuscate/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../server_common/include"

                            ${Boost_INCLUDE_DIRS}
                            ${BOTAN_INCLUDE_DIR}
                            ${ROCKSDB_INCLUDE_DIR}
                            ${LIBRDF_INCLUDE_DIR}
                            ${LIBRDF_RAPTOR_INCLUDE_DIR}
                            ${LIBRDF_RASQAL_INCLUDE_DIR}
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

INSTALL( TARGETS
   keto_rdf_utils

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/rdf_utils" )
