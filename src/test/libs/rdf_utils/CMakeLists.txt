add_executable( rdf_utils_query_parser_test rdf_utils_query_parser_test.cpp)
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling ketod with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../../.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
      OUTPUT_VARIABLE "ketod_BUILD_VERSION"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git commit revision: ${ketod_BUILD_VERSION}")
  else()
    set(ketod_BUILD_VERSION 0)
  endif()
endif()

#configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)

target_include_directories(rdf_utils_query_parser_test PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries( rdf_utils_query_parser_test
        PRIVATE keto_rdf_utils
        PRIVATE keto_chain_common keto_asn1_common keto_crypto keto_server_common keto_environment keto_common keto_asn1_protocol
        PRIVATE ${Boost_LIBRARIES}
        PRIVATE ${LIBRDF_LIBRARY} ${LIBRDF_RASQAL_LIBRARY}
        PRIVATE ${LIBRDF_RAPTOR_LIBRARY}
        PRIVATE ${LIBRDF_EXTRA_LIBRARY} ${LIBRDF_XML_EXTRA_LIBRARY}
        PRIVATE ${LIBRDF_PCRE_EXTRA_LIBRARY} ${LIBRDF_LTDL_EXTRA_LIBRARY}
        PRIVATE ${LIBRDF_GMP_EXTRA_LIBRARY}
        PRIVATE ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
        PRIVATE ${pthread_library} ${BOTAN_LIBRARIES} ${OPENSSL_LIBRARIES} )


install( TARGETS
        rdf_utils_query_parser_test

   RUNTIME DESTINATION test
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
