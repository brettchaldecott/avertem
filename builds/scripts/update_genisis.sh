#!/bin/bash

CURRENT_DIR=$(pwd)
cd ../../src/contracts/keto_standard_typscript_contracts && npm install && npm run asbuild && cd ${CURRENT_DIR}

echo ${CURRENT_DIR}

../../build/install/bin/keto_contract_tools.sh -e -s src/contracts/keto_standard_typscript_contracts/build/base_untouched.wasm -t src/contracts/keto_standard_typscript_contracts/build/base_untouched.hex
BASE_HEX_ID=$(cat src/contracts/keto_standard_typscript_contracts/build/base_untouched.hex)
../../build/install/bin/keto_contract_tools.sh -e -s src/contracts/keto_standard_typscript_contracts/build/fee_untouched.wasm -t src/contracts/keto_standard_typscript_contracts/build/fee_untouched.hex
FEE_HEX_ID=$(cat src/contracts/keto_standard_typscript_contracts/build/fee_untouched.hex)
../../build/install/bin/keto_contract_tools.sh -e -s src/contracts/keto_standard_typscript_contracts/build/nested_untouched.wasm -t src/contracts/keto_standard_typscript_contracts/build/nested_untouched.hex
NESTED_HEX_ID=$(cat src/contracts/keto_standard_typscript_contracts/build/nested_untouched.hex)
cat ../../resources/config/genesis.json.in | sed "s/BASE_CONTRACT/${BASE_HEX_ID}/" | sed "s/FEE_CONTRACT/${FEE_HEX_ID}/" | sed "s/NESTED_CONTRACT/${NESTED_HEX_ID}/" > ../../resources/config/genesis.json

rm -rf ../../build/install/data/*
rm -rf ../../build/install/tmp/*
