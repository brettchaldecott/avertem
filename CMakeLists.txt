# This was original taken from the KETO build
# Defines KETO library target.
project( KETO )
cmake_minimum_required( VERSION 3.1.0 )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/builds/CMakeModules")

INCLUDE( VersionMacros )
INCLUDE( SetupTargetMacros )
INCLUDE( KetoVersionInfo )

SET( BLOCKCHAIN_NAME "KETO" )
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

if( WIN32 )

    message( STATUS "Configuring KETO on WIN32")
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
        message( STATUS "Configuring KETO on OS X" )
#set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -Wall -Wno-invalid-partial-specialization -Wno-deprecated-declarations" )
        set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++14 -Wall -Wno-deprecated-declarations" )
    else( APPLE )
        # Linux Specific Options Here
        message( STATUS "Configuring KETO on Linux" )
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

set(ENABLE_COVERAGE_TESTING FALSE CACHE BOOL "Build KETO for code coverage analysis")

if(ENABLE_COVERAGE_TESTING)
    SET(CMAKE_CXX_FLAGS "--coverage ${CMAKE_CXX_FLAGS}")
endif()

include(wasm)
if(WASM_TOOLCHAIN)
  message(STATUS "Using WASM clang => " ${WASM_CLANG})
  message(STATUS "Using WASM llc => " ${WASM_LLC})
  message(STATUS "Using WASM llvm-link => " ${WASM_LLVM_LINK})
  add_subdirectory(contracts)
else()
  message(STATUS "--------------------------------------------------------------------")
  message(STATUS "No WASM compiler could be found ... (skipping building of contracts)")
  message(STATUS "--------------------------------------------------------------------")
endif()

add_subdirectory( src )
add_subdirectory( scripts )
add_subdirectory( resources )
add_subdirectory( documents )
add_subdirectory( keys )

if (ENABLE_INSTALLER)

include(InstallRequiredSystemLibraries)

set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_BINARY_DIR}/packages)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

SET(CPACK_PACKAGE_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
set(CPACK_PACKAGE_NAME "KETO")
set(CPACK_PACKAGE_VENDOR "BurntJam")
set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION "A client for the KETO network")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A client for the KETO network")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "KETO ${CPACK_PACKAGE_VERSION}")

if(WIN32)
 SET(CPACK_GENERATOR "ZIP;NSIS")
 set(CPACK_PACKAGE_NAME "KETO") # override above
 set(CPACK_NSIS_EXECUTABLES_DIRECTORY .)
 set(CPACK_NSIS_PACKAGE_NAME "KETO v${CPACK_PACKAGE_VERSION}")
 set(CPACK_NSIS_DISPLAY_NAME "${CPACK_NSIS_PACKAGE_NAME}")
 set(CPACK_NSIS_DEFINES "  !define MUI_STARTMENUPAGE_DEFAULTFOLDER \\\"KETO\\\"")
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

 include(CPack)
endif(ENABLE_INSTALLER)

#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/genesis.json ${CMAKE_CURRENT_BINARY_DIR}/genesis.json COPYONLY)

include(FindDoxygen)



if(NOT DOXYGEN_FOUND)
  message(STATUS "Doxygen not found.  Contract documentation will not be generated.")
else()
  message(STATUS "Doxygen found.  Contract documentation will be generated.")
  set(DOXY_KETO_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" CACHE INTERNAL "Version string used in PROJECT_NUMBER.")
  # CMake strips trailing path separators off of variables it knows are paths,
  # so the trailing '/' Doxygen expects is embedded in the doxyfile.
  set(DOXY_DOC_DEST_DIR "${CMAKE_BINARY_DIR}/docs" CACHE PATH "Path to the doxygen output")
  set(DOXY_DOC_INPUT_ROOT_DIR "contracts" CACHE PATH "Path to the doxygen input")
  if(DOXYGEN_DOT_FOUND)
    set(DOXY_HAVE_DOT "YES" CACHE STRING "Doxygen to use dot for diagrams.")
  else(DOXYGEN_DOT_FOUND)
    set(DOXY_HAVE_DOT "NO" CACHE STRING "Doxygen to use dot for diagrams.")
  endif(DOXYGEN_DOT_FOUND)
  configure_file("keto.doxygen.in" "${CMAKE_BINARY_DIR}/eos.doxygen")
  if(BUILD_DOXYGEN) 
     # !! Core developers who don't want to see the doxygen output on every build
     # !! should remove the 'ALL' option from both commands below.
     # Doxygen has issues making destination directories more than one level deep, so do it for it.
     add_custom_target(make_doc_dir ALL COMMAND ${CMAKE_COMMAND} -E make_directory "${DOXY_DOC_DEST_DIR}")
     add_custom_target(contract_documentation ALL
         COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_BINARY_DIR}/keto.doxygen"
         DEPENDS make_doc_dir
         WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
         COMMENT "Building doxygen documentation into ${DOXY_DOC_DEST_DIR}..."
         VERBATIM)
  endif(BUILD_DOXYGEN)
endif()
