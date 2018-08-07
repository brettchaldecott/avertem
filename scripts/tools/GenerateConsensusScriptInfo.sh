#!/bin/bash


if (( $# != 3 )); then
	echo "Must provide the [number] [keyDir] [consensusDir]"
	exit 0
fi

number=$1
keyDir=$2
consensusDir=$3

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

for (( count=1; count<=${number}; count++ ))
do
	
	#echo "${SOURCE_DIR}/../../build/install/bin/keto_tools.sh -H -s \"${consensusDir}/consensus_${count}.chai\""
	#codeHash=`${SOURCE_DIR}/../../build/install/bin/keto_tools.sh -H -s "${consensusDir}/consensus_${count}.chai"`
    codeHashBytes=(`${SOURCE_DIR}/../../build/install/bin/keto_tools.sh -H -s "${consensusDir}/consensus_${count}.chai" | xxd -p -r | od -An -b`)
	#echo "${SOURCE_DIR}/../../build/install/bin/keto_tools.sh -K -k \"${keyDir}/key_${count}.json\""
	privateKeyBytes=(`${SOURCE_DIR}/../../build/install/bin/keto_tools.sh -K -k "${keyDir}/key_${count}.json" | xxd -p -r | od -An -b`)
	#echo "${SOURCE_DIR}/../../build/install/bin/keto_tools.sh -E -k \"${keyDir}/key_${count}.json\" -s \"${consensusDir}/consensus_${count}.chai\""
	encryptedCode=`${SOURCE_DIR}/../../build/install/bin/keto_tools.sh -E -k "${keyDir}/key_${count}.json" -s "${consensusDir}/consensus_${count}.chai"`
	#encryptedCodeBytes=(`${SOURCE_DIR}/../../build/install/bin/keto_tools.sh -E -k "${keyDir}/key_${count}.json" -s "${consensusDir}/consensus_${count}.chai" | xxd -p -r | od -An -b`)
    echo "namespace consensus_code_${count} {
	keto::crypto::SecureVector getHash() {
	    keto::crypto::SecureVector result;"
	for bytes in "${codeHashBytes[@]}";
	do
        echo "	    result.push_back((uint8_t)${bytes});"
	done
	echo "
		return result;
	}

	keto::crypto::SecureVector getEncodedKey() {
	    keto::crypto::SecureVector result;
	"
	for bytes in "${privateKeyBytes[@]}";
	do
        echo "	    result.push_back((uint8_t)${bytes});"
	done

	echo "
		return result;
	}

	std::vector<uint8_t> getCode() {
        std::string code = \"${encryptedCode}\";
        return Botan::hex_decode(code);
    }
}"
#		std::vector<uint8_t> result;"
#
#	for bytes in "${encryptedCodeBytes[@]}";
#	do
#        echo "	    result.push_back((uint8_t)${bytes});"
#	done
#
#	echo "
#		return result;
#	}
#
#}"
done


