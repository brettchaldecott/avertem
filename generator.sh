#!/usr/bin/env bash

##########################################################################
# This is KETO bootstrapper script for Linux and OS X.
##########################################################################

VERSION=1.0
# Define directories.
WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR=${WORK_DIR}/ide_build
TEMP_DIR=/tmp

if [[ $# != 3 ]]; then
	echo "Please supply the architecture and base"
        exit 1
fi;


# Target architectures
ARCH=$1
BASE_HOME=$2
GENERATE_IDE=$3

echo ""
echo ">>> ARCHITECTURE \"$ARCH\""

BOOST_ROOT=${BASE_HOME}/opt/boost_1_66_0
BINARYEN_BIN=${BASE_HOME}/opt/binaryen/bin
OPENSSL_ROOT_DIR=/usr/local/opt/openssl
OPENSSL_LIBRARIES=/usr/local/opt/openssl/lib
WASM_LLVM_CONFIG=${BASE_HOME}/opt/wasm/bin/llvm-config
WASM_LLVM=${BASE_HOME}/opt/wasm/
ROCKSDB_ROOT=${BASE_HOME}/opt/rocksdb
BEAST_ROOT=${BASE_HOME}/opt/beast
BEAST_INCLUDE=${BEAST_ROOT}/include
export BOOST_ROOT BINARYEN_BIN OPENSSL_ROOT_DIR OPENSSL_LIBRARIES WASM_LLVM_CONFIG ROCKSDB_ROOT BEAST_ROOT BEAST_INCLUDE WASM_LLVM

PROTOBUF_SRC_ROOT_FOLDER=${BASE_HOME}/opt/protobuf/
PROTOBUF_LIBRARY=${PROTOBUF_SRC_ROOT_FOLDER}/lib/
PROTOBUF_IMPORT_DIRS=${PROTOBUF_SRC_ROOT_FOLDER}/include/
export PROTOBUF_SRC_ROOT_FOLDER PROTOBUF_LIBRARY PROTOBUF_IMPORT_DIRS

ASN1_ROOT_FOLDER=${BASE_HOME}/opt/asn1c/
export ASN1_ROOT_FOLDER

BOTAN_ROOT=${BASE_HOME}/opt/botan/
export BOTAN_ROOT

ROCKSDB_ROOT=${BASE_HOME}/opt/rocksdb/
export ROCKSDB_ROOT

LIBRDF_ROOT=${BASE_HOME}/opt/librdf/
export LIBRDF_ROOT

JSON_ROOT=${BASE_HOME}/opt/nlohmann/
export JSON_ROOT

CHAI_SCRIPT_ROOT=${BASE_HOME}/opt/ChaiScript/
export CHAI_SCRIPT_ROOT

LIBWAVM_ROOT=${BASE_HOME}/opt/wavm/
export LIBWAVM_ROOT

LIBWALLY_ROOT=${BASE_HOME}/opt/libwally-core/
export LIBWALLY_ROOT

# Debug flags
COMPILE_KETO=1
COMPILE_CONTRACTS=1

# Define default arguments.
CMAKE_BUILD_TYPE=RelWithDebugInfo

CXX_COMPILER=clang++-4.0
C_COMPILER=clang-4.0

if [ $ARCH == "darwin" ]; then
  CXX_COMPILER=clang++
  C_COMPILER=clang
fi

# Create the build dir
#cd ${WORK_DIR}
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# Build KETO 
#cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} -DCMAKE_C_COMPILER=${C_COMPILER} -DWASM_LLVM_CONFIG=${WASM_LLVM_CONFIG} -DWASM_LLVM=${WASM_LLVM} -DBINARYEN_BIN=${BINARYEN_BIN} -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR} -DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES} --build ../src/programs/tools/
cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} -DCMAKE_C_COMPILER=${C_COMPILER} -DWASM_LLVM_CONFIG=${WASM_LLVM_CONFIG} -DWASM_LLVM=${WASM_LLVM} -DBINARYEN_BIN=${BINARYEN_BIN} -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR} -DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES} -G "${GENERATE_IDE}" ..

mv src src.bak
ln -s ../build/src src
