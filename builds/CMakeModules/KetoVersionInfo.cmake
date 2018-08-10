macro(KetoModuleDepencencies MODULE_NAME)
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
    #MESSAGE("The header ${headerList}")
    #MESSAGE("The source class ${sourceClassList}")
    #MESSAGE("The header class ${headerClassList}")
    
    set ("${MODULE_NAME}_headerList" "${headerList}")
    #MESSAGE("The header class ${0015_account_module_headerClassList}")
    set ("${MODULE_NAME}_headerClassList" "${headerClassList}")
    set ("${MODULE_NAME}_sourceClassList" "${sourceClassList}")
    
    target_link_libraries( ${MODULE_NAME} ${ARGN} )
endmacro(KetoModuleDepencencies)

macro(KetoModuleConsensus MODULE_NAME NUMBER_OF_KEYS MODULE_NAMESPACE  )
    message("${CMAKE_BINARY_DIR}/../scripts/tools/GenerateConsensusScriptInfo.sh" "${NUMBER_OF_KEYS}" "${CMAKE_CURRENT_SOURCE_DIR}/keys/" "${CMAKE_CURRENT_SOURCE_DIR}/consensus/")
    execute_process(COMMAND "${CMAKE_BINARY_DIR}/../scripts/tools/GenerateConsensusScriptInfo.sh" "${NUMBER_OF_KEYS}" "${CMAKE_CURRENT_SOURCE_DIR}/keys/" "${CMAKE_CURRENT_SOURCE_DIR}/consensus/" OUTPUT_VARIABLE consensusSource)
    execute_process(COMMAND "${CMAKE_BINARY_DIR}/../scripts/tools/GenerateConsensusScriptMapping.sh" "${NUMBER_OF_KEYS}"  "${MODULE_NAMESPACE}" OUTPUT_VARIABLE consensusMapping)
    
    set ("${MODULE_NAME}_consensus_scripts" "${consensusSource}")
    set ("${MODULE_NAME}_consensus_mapping" "${consensusMapping}")
endmacro(KetoModuleConsensus)

macro(KetoConsensusKeys NUMBER_OF_KEYS)

    execute_process(COMMAND "${CMAKE_BINARY_DIR}/../scripts/tools/KetoConsensusKeys.sh" "${NUMBER_OF_KEYS}" "${CMAKE_BINARY_DIR}/../src/resources/keys/" OUTPUT_VARIABLE KetoConsensusKeys)

endmacro(KetoConsensusKeys)