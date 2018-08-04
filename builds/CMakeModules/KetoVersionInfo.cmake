macro(KetoModuleDepencencies MODULE_NAME)
    MESSAGE("The depencencies ${ARGN}")
    set(list_var "${ARGN}")
    
    set (headerList "")
    set (sourceClassList "")
    set (headerClassList "")
    
    foreach(loop_var IN LISTS list_var)
        if (${loop_var} MATCHES "keto_.*")
            #MESSAGE("The argument to the module dependencies ${loop_var}")
            #MESSAGE("${CMAKE_BINARY_DIR}/../scripts/tools/GenerateVersionMappingsForLibrary.sh ${CMAKE_BINARY_DIR}/../src/libs/ ${loop_var} -h")
            execute_process(COMMAND "${CMAKE_BINARY_DIR}/../scripts/tools/GenerateVersionMappingsForLibrary.sh" "${CMAKE_BINARY_DIR}/../src/libs/" "${loop_var}" "-h" OUTPUT_VARIABLE newHeaderList)
            SET(headerList "${headerList}\n${newHeaderList}")
            execute_process(COMMAND "${CMAKE_BINARY_DIR}/../scripts/tools/GenerateVersionMappingsForLibrary.sh" "${CMAKE_BINARY_DIR}/../src/libs/" "${loop_var}" "-sc" OUTPUT_VARIABLE newSourceClassList)
            SET(sourceClassList "${sourceClassList}\n${newSourceClassList}")
            execute_process(COMMAND "${CMAKE_BINARY_DIR}/../scripts/tools/GenerateVersionMappingsForLibrary.sh" "${CMAKE_BINARY_DIR}/../src/libs/" "${loop_var}" "-hc" OUTPUT_VARIABLE newHeaderClassList)
            SET(headerClassList "${headerClassList}\n${newHeaderClassList}")
        endif (${loop_var} MATCHES "keto_.*")
    ENDFOREACH( loop_var )
    MESSAGE("The header ${headerList}")
    MESSAGE("The source class ${sourceClassList}")
    MESSAGE("The header class ${headerClassList}")
    
    set ("${MODULE_NAME}_headerList" "${headerList}")
    #MESSAGE("The header class ${0015_account_module_headerClassList}")
    set ("${MODULE_NAME}_headerClassList" "${headerClassList}")
    set ("${MODULE_NAME}_sourceClassList" "${sourceClassList}")
    
    target_link_libraries( ${MODULE_NAME} ${ARGN} )
endmacro(KetoModuleDepencencies)