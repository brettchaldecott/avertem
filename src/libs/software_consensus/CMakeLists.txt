file(GLOB HEADERS "include/keto/software_consensus/*.hpp")

include_directories(${PROTOBUF_INCLUDE_DIR})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

## SORT .cpp by most likely to change / break compile
add_library( keto_software_consensus
            
              SoftwareMerkelUtils.cpp
	      ConsensusBuilder.cpp
              ConsensusMessageHelper.cpp
              ConsensusSessionManager.cpp
              Constants.cpp
              HashBuilder.cpp
              SoftwareConsensusHelper.cpp
              ConsensusHashScriptInfo.cpp
              ConsensusHashGenerator.cpp
              ModuleConsensusHelper.cpp
              ModuleConsensusGenerator.cpp
              ModuleSessionMessageHelper.cpp
              ModuleHashMessageHelper.cpp

             ${HEADERS}
           )

target_link_libraries( keto_software_consensus)
target_include_directories( keto_software_consensus
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../environment/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../event_service_api/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../transaction_manager_api/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../module_manager/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../asn1/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../server_common/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../crypto/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../obfuscate/include"
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../key_tools/include"
                            
                            PUBLIC "${CMAKE_BINARY_DIR}/src/protocol/proto/"
                            PUBLIC "${CMAKE_BINARY_DIR}/src/protocol/asn1/" 
                            PUBLIC "${CMAKE_BINARY_DIR}/src/libs/common/include/" 
                            PUBLIC "${PROTO_HDRS}"

                            ${Boost_INCLUDE_DIRS}
                            ${BOTAN_INCLUDE_DIR}
                            ${CHAI_SCRIPT_INCLUDE_DIR}
                            )

#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

INSTALL( TARGETS
   keto_software_consensus

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/keto/software_consensus" )
