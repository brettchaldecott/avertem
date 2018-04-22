# - Try to find the Librdf library
#
# Once done this will define
#
#  LIBWAVM_FOUND - System has 
#  LIBWAVM_INCLUDE_DIR - The librdf include directory
#  LIBWAVM_LIBRARIES - The libraries needed to use rdf

if (NOT DEFINED LIBWAVM_ROOT)
  set(LIBWAVM_ROOT_FOLDER $ENV{LIBWAVM_ROOT})
endif()

IF (LIBWAVM_INCLUDE_DIR AND LIBWAVM_LIBRARY)
   # in cache already
   SET(LibWavm_FIND_QUIETLY TRUE)
ENDIF (LIBWAVM_INCLUDE_DIR AND LIBWAVM_LIBRARY)

IF (NOT WIN32)
   # try using pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   # also fills in LIBWAVM_DEFINITIONS, although that isn't normally useful
   FIND_PACKAGE(PkgConfig)
   PKG_SEARCH_MODULE(PC_LIBWAVM libwavm)
   SET(LIBWAVM_DEFINITIONS ${PC_LIBWAVM_CFLAGS})
ENDIF (NOT WIN32)


# attempt to find the botan include directory using the standard KETO build process
# fall back to the installed packages
FIND_PATH(LIBWAVM_INCLUDE_DIR "Runtime/Runtime.h"
    PATHS "${LIBWAVM_ROOT_FOLDER}/include/")

IF (NOT LIBWAVM_INCLUDE_DIR)
    FIND_PATH(LIBWAVM_INCLUDE_DIR "Runtime/Runtime.h"
        HINTS
        ${PC_LIBWAVM_INCLUDEDIR}
        ${PC_LIBWAVM_INCLUDE_DIRS}
        )
ENDIF (NOT LIBWAVM_INCLUDE_DIR)

# find the library, first be optimist and this is being built with a standard KETO build,
# if not found fall back to the possible package install
find_library(LIBWAVM_PLATFORM_LIBRARY
    NAMES Platform
    PATHS ${LIBWAVM_ROOT_FOLDER}/lib)

find_library(LIBWAVM_RUNTIME_LIBRARY
    NAMES Runtime
    PATHS ${LIBWAVM_ROOT_FOLDER}/lib)

find_library(LIBWAVM_LIBRARY
    NAMES WASM
    PATHS ${LIBWAVM_ROOT_FOLDER}/lib)

find_library(LIBWAVM_WAST_LIBRARY
    NAMES WAST
    PATHS ${LIBWAVM_ROOT_FOLDER}/lib)

find_library(LIBWAVM_LOGGING_LIBRARY
    NAMES Logging
    PATHS ${LIBWAVM_ROOT_FOLDER}/lib)

find_library(LIBWAVM_IR_LIBRARY
    NAMES IR
    PATHS ${LIBWAVM_ROOT_FOLDER}/lib)

find_library(LIBWAVM_EMSCRIPTEN_LIBRARY
    NAMES Emscripten
    PATHS ${LIBWAVM_ROOT_FOLDER}/lib)

# Find an installed build of LLVM
find_package(LLVM 6.0 REQUIRED CONFIG PATHS ${WASM_LLVM})

# Include the LLVM headers

llvm_map_components_to_libnames(LIBWAVM_LLVM_LIBS support core passes mcjit native DebugInfoDWARF)


message(STATUS "LIBWAVM_INCLUDE_DIR " ${LIBWAVM_INCLUDE_DIR})
message(STATUS "LIBWAVM_PLATFORM_LIBRARY " ${LIBWAVM_PLATFORM_LIBRARY})
message(STATUS "LIBWAVM_RUNTIME_LIBRARY " ${LIBWAVM_RUNTIME_LIBRARY})
message(STATUS "LIBWAVM_LIBRARY " ${LIBWAVM_LIBRARY})
message(STATUS "LIBWAVM_WAST_LIBRARY " ${LIBWAVM_WAST_LIBRARY})
message(STATUS "LIBWAVM_LOGGING_LIBRARY " ${LIBWAVM_LOGGING_LIBRARY})
message(STATUS "LIBWAVM_IR_LIBRARY " ${LIBWAVM_IR_LIBRARY})
message(STATUS "LIBWAVM_EMSCRIPTEN_LIBRARY " ${LIBWAVM_EMSCRIPTEN_LIBRARY})
message(STATUS "LLVM_INCLUDE_DIRS " ${LLVM_INCLUDE_DIRS})
message(STATUS "LLVM_DEFINITIONS " ${LLVM_DEFINITIONS})
message(STATUS "LIBWAVM_LLVM_LIBS " ${LIBWAVM_LLVM_LIBS})

MARK_AS_ADVANCED(LIBWAVM_INCLUDE_DIR LIBWAVM_PLATFORM_LIBRARY LIBWAVM_RUNTIME_LIBRARY LIBWAVM_LIBRARY LIBWAVM_WAST_LIBRARY LIBWAVM_LOGGING_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set LIBWAVM_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Libwavm DEFAULT_MSG LIBWAVM_PLATFORM_LIBRARY LIBWAVM_RUNTIME_LIBRARY LIBWAVM_LIBRARY LIBWAVM_WAST_LIBRARY LIBWAVM_LOGGING_LIBRARY)

IF(LIBWAVM_FOUND)
    SET(LIBWAVM_INCLUDE_DIR    ${LIBWAVM_INCLUDE_DIR})
    SET(LIBWAVM_PLATFORM_LIBRARY    ${LIBWAVM_PLATFORM_LIBRARY})
    SET(LIBWAVM_RUNTIME_LIBRARY    ${LIBWAVM_RUNTIME_LIBRARY})
    SET(LIBWAVM_LIBRARY ${LIBWAVM_LIBRARY})
    SET(LIBWAVM_WAST_LIBRARY ${LIBWAVM_WAST_LIBRARY})
    SET(LIBWAVM_LOGGING_LIBRARY ${LIBWAVM_LOGGING_LIBRARY})
    SET(LIBWAVM_IR_LIBRARY ${LIBWAVM_IR_LIBRARY})
    SET(LIBWAVM_EXTRA_LIBRARY ${LIBWAVM_EXTRA_LIBRARY})
    SET(LIBWAVM_EMSCRIPTEN_LIBRARY ${LIBWAVM_EMSCRIPTEN_LIBRARY})
    SET(LIBWAVM_LLVM_INCLUDE_DIRS ${LLVM_INCLUDE_DIRS})
    SET(LIBWAVM_LLVM_LIBS ${LIBWAVM_LLVM_LIBS})
    add_definitions(${LLVM_DEFINITIONS})

ENDIF()
