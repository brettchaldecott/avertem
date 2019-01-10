#!/bin/bash


if (( $# != 3 )); then
	echo "Must provide the [number] [keyDir] [consensusDir]"
	exit 0
fi

number=$1
keyDir=$2
consensusDir=$3

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

DEPS_BUILD=deps_build

if [ ! -f "${SOURCE_DIR}/../../${DEPS_BUILD}/build/install/bin/keto_tools.sh" ]; then
	exit 0
fi

for (( count=1; count<=${number}; count++ ))
do
	
	#echo "${SOURCE_DIR}/../../${DEPS_BUILD}/build/install/bin/keto_tools.sh -H -s \"${consensusDir}/consensus_${count}.chai\""
	codeHash=`${SOURCE_DIR}/../../${DEPS_BUILD}/build/install/bin/keto_tools.sh -H -s "${consensusDir}/consensus_${count}.chai"`
    #codeHashBytes=(`${SOURCE_DIR}/../../${DEPS_BUILD}/build/install/bin/keto_tools.sh -H -s "${consensusDir}/consensus_${count}.chai" | xxd -p -r | od -An -b`)
	#echo "${SOURCE_DIR}/../../${DEPS_BUILD}/build/install/bin/keto_tools.sh -K -k \"${keyDir}/key_${count}.json\""
	privateKeyBytes=`${SOURCE_DIR}/../../${DEPS_BUILD}/build/install/bin/keto_tools.sh -K -k "${keyDir}/key_${count}.json"`
	#echo "${SOURCE_DIR}/../../${DEPS_BUILD}/build/install/bin/keto_tools.sh -E -k \"${keyDir}/key_${count}.json\" -s \"${consensusDir}/consensus_${count}.chai\""
	encryptedCode=`${SOURCE_DIR}/../../${DEPS_BUILD}/build/install/bin/keto_tools.sh -E -k "${keyDir}/key_${count}.json" -s "${consensusDir}/consensus_${count}.chai"`
	encryptedShortCode=`${SOURCE_DIR}/../../${DEPS_BUILD}/build/install/bin/keto_tools.sh -E -k "${keyDir}/key_${count}.json" -s "${consensusDir}/consensus_${count}.chai"`
	#encryptedCodeBytes=(`${SOURCE_DIR}/../../${DEPS_BUILD}/build/install/bin/keto_tools.sh -E -k "${keyDir}/key_${count}.json" -s "${consensusDir}/consensus_${count}.chai" | xxd -p -r | od -An -b`)
	#echo "Hash ${codeHash} ${privateKeyBytes} ${encryptedCode}"
    echo "namespace consensus_code_${count} {
	keto::crypto::SecureVector getHash() {
        std::string hash = OBFUSCATED(\"${codeHash}\");
        return Botan::hex_decode_locked(hash);
	}

	keto::crypto::SecureVector getEncodedKey() {
        std::string encodedKey = \"${privateKeyBytes}\";
        return Botan::hex_decode_locked(encodedKey);
	}

	std::vector<uint8_t> getCode() {
        std::string code = \"${encryptedCode}\";
        return Botan::hex_decode(code);
    }
	std::vector<uint8_t> getShortCode() {
        std::string code = \"${encryptedShortCode}\";
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


