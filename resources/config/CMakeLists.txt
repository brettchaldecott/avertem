
KetoConsensusKeys(${NUMBER_OF_KEYS})

configure_file(config.ini.in config.ini)

install(FILES "${CMAKE_BINARY_DIR}/resources/config/config.ini" cli_config.ini
    avertem_contract_tools.ini genesis.json avertem_version
    auto_upgrade
     DESTINATION config)
