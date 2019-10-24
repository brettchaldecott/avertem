#!/bin/bash

HAS_NPM="$(which npm ; echo $?)"
if [ "${HAS_NPM}" == "1" ] ; then
    cd /tmp
    apt-get install curl
    curl -sL https://deb.nodesource.com/setup_12.x | bash -
    apt-get install nodejs
fi

WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORK_DIR=${WORK_DIR}/../../
echo "Work dir ${WORK_DIR}"
ls ${WORK_DIR}
cd ${WORK_DIR}/src/contracts/keto_standard_typscript_contracts && npm install && npm run asbuild && cd ${CURRENT_DIR}

echo ${CURRENT_DIR}

${WORK_DIR}/build/install/bin/keto_contract_tools.sh -e -s ${WORK_DIR}/src/contracts/keto_standard_typscript_contracts/build/base_untouched.wasm -t ${WORK_DIR}/src/contracts/keto_standard_typscript_contracts/build/base_untouched.hex
BASE_HEX_ID=$(cat ${WORK_DIR}/src/contracts/keto_standard_typscript_contracts/build/base_untouched.hex)
${WORK_DIR}/build/install/bin/keto_contract_tools.sh -e -s ${WORK_DIR}/src/contracts/keto_standard_typscript_contracts/build/fee_untouched.wasm -t ${WORK_DIR}/src/contracts/keto_standard_typscript_contracts/build/fee_untouched.hex
FEE_HEX_ID=$(cat ${WORK_DIR}/src/contracts/keto_standard_typscript_contracts/build/fee_untouched.hex)
${WORK_DIR}/build/install/bin/keto_contract_tools.sh -e -s ${WORK_DIR}/src/contracts/keto_standard_typscript_contracts/build/nested_untouched.wasm -t ${WORK_DIR}/src/contracts/keto_standard_typscript_contracts/build/nested_untouched.hex
NESTED_HEX_ID=$(cat ${WORK_DIR}/src/contracts/keto_standard_typscript_contracts/build/nested_untouched.hex)
cat ${WORK_DIR}/resources/config/genesis.json.in | sed "s/BASE_CONTRACT/${BASE_HEX_ID}/" | sed "s/FEE_CONTRACT/${FEE_HEX_ID}/" | sed "s/NESTED_CONTRACT/${NESTED_HEX_ID}/" > ${WORK_DIR}/resources/config/genesis.json

rm -rf ../../build/install/data/*
rm -rf ../../build/install/tmp/*
