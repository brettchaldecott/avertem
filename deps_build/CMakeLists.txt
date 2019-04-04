# This was original taken from the KETO_DEPS build
# Defines KETO_DEPS library target.
project( KETO_DEPS )
cmake_minimum_required( VERSION 3.1.0 )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../builds/CMakeModules")

INCLUDE( VersionMacros )
INCLUDE( SetupTargetMacros )
INCLUDE( KetoVersionInfo )

SET( BLOCKCHAIN_NAME "KETO_DEPS" )
SET( CMAKE_CXX_STANDARD 14 )
SET( NUMBER_OF_KEYS 24 )

SET(VERSION_MAJOR 0)
SET(VERSION_MINOR 0)
SET(VERSION_PATCH 1)
SET(PROTOCOL_VERSION 1)

SET(COPY_RIGHT MIT)

#SET( CLI_CLIENT_EXECUTABLE_NAME keto_client )
#SET( GUI_CLIENT_EXECUTABLE_NAME keto )
#SET( CUSTOM_URL_SCHEME "gcs" )
#SET( INSTALLER_APP_ID "68ad7005-8eee-49c9-95ce-9eed97e5b347" )

# http://stackoverflow.com/a/18369825
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        message(FATAL_ERROR "GCC version must be at least 4.8!")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
        message(FATAL_ERROR "Clang version must be at least 3.3!")
    endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
set(BUILD_DOXYGEN TRUE CACHE BOOL "Build doxygen documentation on every make")
#set(EOS_EGENESIS_JSON "${CMAKE_CURRENT_SOURCE_DIR}/genesis.json" )

set (ENABLE_INSTALLER 1)
set (USE_PCH 1)

if (USE_PCH)
  include (cotire)
endif(USE_PCH)

SET(BOOST_COMPONENTS)
LIST(APPEND BOOST_COMPONENTS 
                            log_setup
                            log
                             date_time
                             system
                             filesystem
                             program_options
                             signals
                             serialization
                             chrono
                             coroutine
                             unit_test_framework
                             context
                             locale
                             regex
                            thread
                            )

SET( Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF" )

IF( WIN32 )
  SET(BOOST_ROOT $ENV{BOOST_ROOT})
  set(Boost_USE_MULTITHREADED ON)
  set(BOOST_ALL_DYN_LINK OFF) # force dynamic linking for all libraries
ENDIF(WIN32)

FIND_PACKAGE(Boost 1.66 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
# For Boost 1.53 on windows, coroutine was not in BOOST_LIBRARYDIR and do not need it to build,  but if boost versin >= 1.54, find coroutine otherwise will cause link errors
IF(NOT "${Boost_VERSION}" MATCHES "1.53(.*)")
   SET(BOOST_LIBRARIES_TEMP ${Boost_LIBRARIES})
   FIND_PACKAGE(Boost 1.54 REQUIRED COMPONENTS coroutine)
   LIST(APPEND BOOST_COMPONENTS coroutine)
   SET(Boost_LIBRARIES ${BOOST_LIBRARIES_TEMP} ${Boost_LIBRARIES})
ENDIF()

find_package(OpenSSL REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Asn1 REQUIRED)
find_package(Botan REQUIRED)
find_package(RocksDB REQUIRED)
find_package(Librdf REQUIRED)
find_package(Json REQUIRED)
find_package(Chaiscript REQUIRED)
find_package(LibWavm REQUIRED)
find_package(LibWally REQUIRED)

if( WIN32 )

    message( STATUS "Configuring KETO_DEPS on WIN32")
    set( DB_VERSION 60 )
    set( BDB_STATIC_LIBS 1 )

    set( ZLIB_LIBRARIES "" )
    SET( DEFAULT_EXECUTABLE_INSTALL_DIR bin/ )

    set(CRYPTO_LIB)

    #looks like this flag can have different default on some machines.
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")

    # Probably cmake has a bug and vcxproj generated for executable in Debug conf. has disabled debug info
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")

    # On windows tcl should be installed to the directory pointed by setenv.bat script
    SET(TCL_INCLUDE_PATH $ENV{TCL_ROOT}/include)
    MESSAGE(STATUS "tcl INCLUDE PATH: ${TCL_INCLUDE_PATH}")

    FIND_PACKAGE(TCL)
    MESSAGE(STATUS "tcl_library: ${TCL_LIBRARY}")

    SET(TCL_LIBS "optimized;${TCL_LIBRARY};debug;")
    get_filename_component(TCL_LIB_PATH "${TCL_LIBRARY}" PATH)
    get_filename_component(TCL_LIB_NAME "${TCL_LIBRARY}" NAME_WE)
    get_filename_component(TCL_LIB_EXT "${TCL_LIBRARY}" EXT)

    SET(TCL_LIBS "${TCL_LIBS}${TCL_LIB_PATH}/${TCL_LIB_NAME}g${TCL_LIB_EXT}")
    SET(TCL_LIBRARY ${TCL_LIBS})

else( WIN32 ) # Apple AND Linux

    find_library(READLINE_LIBRARIES NAMES readline)
    find_path(READLINE_INCLUDE_DIR readline/readline.h)
    #if(NOT READLINE_INCLUDE_DIR OR NOT READLINE_LIBRARIES)
    #  MESSAGE(FATAL_ERROR "Could not find lib readline.")
    #endif()

    if( APPLE )
        # Apple Specific Options Here
        message( STATUS "Configuring KETO_DEPS on OS X" )
#set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -Wall -Wno-invalid-partial-specialization -Wno-deprecated-declarations" )
        set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -Wall -Wno-deprecated-declarations" )
    else( APPLE )
        # Linux Specific Options Here
        message( STATUS "Configuring KETO_DEPS on Linux" )
        set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} --std=c++14 -O3 -Wall -Wno-unknown-pragmas" )
        set( rt_library rt )
        set( pthread_library pthread)
        if ( NOT DEFINED crypto_library )
          # I'm not sure why this is here, I guess someone has openssl and can't detect it with find_package()?
          # if you have a normal install, you can define crypto_library to the empty string to avoid a build error
          set( crypto_library crypto)
        endif ()
        if ( FULL_STATIC_BUILD )
          set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
        endif ( FULL_STATIC_BUILD )

        if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
            if( CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.0.0 OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.0.0 )
                set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-partial-specialization" )
            endif()
        endif()
    endif( APPLE )

    if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp" )
    endif()

    if( "${CMAKE_GENERATOR}" STREQUAL "Ninja" )
        if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics" )
        endif()
    endif()

    # based on http://www.delorie.com/gnu/docs/gdb/gdb_70.html
    # uncomment this line to tell GDB about macros (slows compile times)
    # set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-2 -g3" )

endif( WIN32 )

set(ENABLE_COVERAGE_TESTING FALSE CACHE BOOL "Build KETO_DEPS for code coverage analysis")

if(ENABLE_COVERAGE_TESTING)
    SET(CMAKE_CXX_FLAGS "--coverage ${CMAKE_CXX_FLAGS}")
endif()

add_subdirectory( ../src/protocol/proto/
    "${CMAKE_CURRENT_BINARY_DIR}/src/protocol/proto/")
add_subdirectory( ../src/protocol/asn1/
    "${CMAKE_CURRENT_BINARY_DIR}/src/protocol/asn1/")
add_subdirectory( ../src/libs/environment/
    "${CMAKE_CURRENT_BINARY_DIR}/src/libs/environment/")
add_subdirectory( ../src/libs/common/
    "${CMAKE_CURRENT_BINARY_DIR}/src/libs/common/")
add_subdirectory( ../src/libs/crypto/ 
    "${CMAKE_CURRENT_BINARY_DIR}/src/libs/crypto/")
add_subdirectory( ../src/libs/server_common/ 
    "${CMAKE_CURRENT_BINARY_DIR}/src/libs/server_common/")
add_subdirectory( ../src/libs/key_tools/
    "${CMAKE_CURRENT_BINARY_DIR}/src/libs/key_tools/")
add_subdirectory( ../src/programs/tools/
    "${CMAKE_CURRENT_BINARY_DIR}/src/programs/tools/")
add_subdirectory( ../scripts/wrappers/ 
    "${CMAKE_CURRENT_BINARY_DIR}/scripts/wrappers/")
add_subdirectory( ../resources/config/
    "${CMAKE_CURRENT_BINARY_DIR}/resources/config/")

if (ENABLE_INSTALLER)

include(InstallRequiredSystemLibraries)

set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_BINARY_DIR}/packages)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

SET(CPACK_PACKAGE_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
set(CPACK_PACKAGE_NAME "KETO_DEPS")
set(CPACK_PACKAGE_VENDOR "BurntJam")
set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION "A client for the KETO_DEPS network")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A client for the KETO_DEPS network")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "KETO_DEPS ${CPACK_PACKAGE_VERSION}")

if(WIN32)
 SET(CPACK_GENERATOR "ZIP;NSIS")
 set(CPACK_PACKAGE_NAME "KETO_DEPS") # override above
 set(CPACK_NSIS_EXECUTABLES_DIRECTORY .)
 set(CPACK_NSIS_PACKAGE_NAME "KETO_DEPS v${CPACK_PACKAGE_VERSION}")
 set(CPACK_NSIS_DISPLAY_NAME "${CPACK_NSIS_PACKAGE_NAME}")
 set(CPACK_NSIS_DEFINES "  !define MUI_STARTMENUPAGE_DEFAULTFOLDER \\\"KETO_DEPS\\\"")
 # it seems like windows zip files usually don't have a single directory inside them, unix tgz frequently do
 SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

endif(WIN32)

if(APPLE)
  set(CPACK_GENERATOR "DragNDrop")
endif()

if(LINUX)
  # Linux gets a .tgz
  SET(CPACK_GENERATOR "TGZ")
  SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)
endif(LINUX)

endif(ENABLE_INSTALLER)

#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/genesis.json ${CMAKE_CURRENT_BINARY_DIR}/genesis.json COPYONLY)

