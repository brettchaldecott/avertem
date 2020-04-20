add_executable( avertemd main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling avertemd with TCMalloc")
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

target_include_directories(avertemd PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries( avertemd
        PRIVATE keto_environment keto_common keto_module_manager
        PRIVATE ${Boost_LIBRARIES}
        PRIVATE ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
        PRIVATE ${pthread_library}
        )


install( TARGETS
   avertemd

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
