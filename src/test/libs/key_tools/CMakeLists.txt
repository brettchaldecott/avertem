add_executable( key_tools_test key_tools_test.cpp )
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

target_include_directories(key_tools_test 
        PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
        PUBLIC ${CHAI_SCRIPT_INCLUDE_DIR})

target_link_libraries( key_tools_test
        PRIVATE keto_key_tools keto_software_consensus keto_server_common
        PRIVATE keto_chain_common keto_asn1_common keto_crypto keto_environment keto_common keto_asn1_protocol
        PRIVATE ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
        PRIVATE ${pthread_library} ${BOTAN_LIBRARIES} ${OPENSSL_LIBRARIES} )


install( TARGETS
   key_tools_test

   RUNTIME DESTINATION test
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
