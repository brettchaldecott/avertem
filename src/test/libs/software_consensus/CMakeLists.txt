# parse the configuration file
SET(TEST_MODULE_SIGNATURE "test module signature")
SET(TEST_MODULE_KEY "test module key")

configure_file(include/keto/software_consensus/hash_generator_test_config.hpp.in include/keto/software_consensus/hash_generator_test_config.hpp)

add_executable( software_consensus_test hash_builder_test.cpp )
add_executable( software_consensus_hash_test hash_generator_test.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling ketod with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

#configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)

target_include_directories(software_consensus_test 
        PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
        PUBLIC ${CHAI_SCRIPT_INCLUDE_DIR})

target_link_libraries( software_consensus_test 
        PRIVATE keto_software_consensus keto_server_common
        PRIVATE keto_chain_common keto_asn1_common keto_crypto keto_environment keto_common keto_asn1_protocol
        PRIVATE ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
        PRIVATE ${pthread_library} ${BOTAN_LIBRARIES} ${OPENSSL_LIBRARIES} )

target_include_directories(software_consensus_hash_test 
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
        PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
        PUBLIC ${CHAI_SCRIPT_INCLUDE_DIR})

target_link_libraries( software_consensus_hash_test 
        PRIVATE keto_software_consensus keto_key_tools keto_server_common
        PRIVATE keto_chain_common keto_asn1_common keto_crypto keto_environment keto_common keto_asn1_protocol
        PRIVATE ${Boost_LIBRARIES} ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
        PRIVATE ${pthread_library} ${BOTAN_LIBRARIES} ${OPENSSL_LIBRARIES} )


install( TARGETS
   software_consensus_test
   software_consensus_hash_test

   RUNTIME DESTINATION test
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
