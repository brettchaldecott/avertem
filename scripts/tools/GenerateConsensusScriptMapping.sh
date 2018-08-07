#!/bin/bash


if (( $# != 2 )); then
	echo "Must provide the [number] [module]"
	exit 0
fi

number=$1
module=$2

for (( count=1; count<=${number}; count++ ))
do
	
    echo "
    consensusHashScript.push_back(std::make_shared<keto::software_consensus::ConsensusHashScriptInfo>(
                &keto::${module}::consensus_code_${count}::getHash,
                &keto::${module}::consensus_code_${count}::getEncodedKey,
                &keto::${module}::consensus_code_${count}::getCode));"
done


