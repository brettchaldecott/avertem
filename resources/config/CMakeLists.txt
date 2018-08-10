
KetoConsensusKeys(${NUMBER_OF_KEYS})

configure_file(config.ini.in config.ini)

install(FILES "${CMAKE_BINARY_DIR}/resources/config/config.ini" cli_config.ini keto_contract_tools.ini genesis.json helloworld.wast
     DESTINATION config)
