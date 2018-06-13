# - Try to find the Botan library
#
# Once done this will define
#
#  CHAI_SCRIPT_FOUND - System has Botan
#  CHAI_SCRIPT_INCLUDE_DIR - The Botan include directory

if (NOT DEFINED CHAI_SCRIPT_ROOT)
  set(CHAI_SCRIPT_ROOT_FOLDER $ENV{CHAI_SCRIPT_ROOT})
endif()

IF (CHAI_SCRIPT_INCLUDE_DIR)
   # in cache already
   SET(Chaiscript_FIND_QUIETLY TRUE)
ENDIF (CHAI_SCRIPT_INCLUDE_DIR)

# attempt to find the botan include directory using the standard KETO build process
# fall back to the installed packages
FIND_PATH(CHAI_SCRIPT_INCLUDE_DIR chaiscript/chaiscript.hpp
    PATHS "${CHAI_SCRIPT_ROOT_FOLDER}/include/"
)

message(STATUS "CHAI_SCRIPT_INCLUDE_DIR " ${CHAI_SCRIPT_INCLUDE_DIR})

MARK_AS_ADVANCED(CHAI_SCRIPT_INCLUDE_DIR)

# handle the QUIETLY and REQUIRED arguments and set CHAI_SCRIPT_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Chaiscript DEFAULT_MSG CHAI_SCRIPT_INCLUDE_DIR)

IF(CHAI_SCRIPT_FOUND)
    SET(CHAI_SCRIPT_INCLUDE_DIRS ${CHAI_SCRIPT_INCLUDE_DIR})
ENDIF()
