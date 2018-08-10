
KetoConsensusKeys(${NUMBER_OF_KEYS})

configure_file(config.ini.in config.ini)

install(FILES config.ini.in cli_config.ini keto_contract_tools.ini genesis.json helloworld.wast
     DESTINATION config)