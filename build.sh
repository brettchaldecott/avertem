#!/usr/bin/env bash

##########################################################################
# This is KETO bootstrapper script for Linux and OS X.
##########################################################################

VERSION=1.0
ulimit -u
# Define directories.
WORK_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR=${WORK_DIR}/build
BUILD_DEP_DIR=${WORK_DIR}/deps_build/build/
TEMP_DIR=/tmp


# Target architectures
ARCH=$1
TARGET_ARCHS="ubuntu darwin"

# Check ARCH
if [[ $# > 2 ]]; then
  echo ""
  echo "Error: too many arguments"
  exit 1
fi

if [[ $# < 1 ]]; then
  echo ""
  echo "Usage: bash build.sh TARGET [full|build]"
  echo ""
  echo "Targets: $TARGET_ARCHS"
  exit 1
fi

if [[ $ARCH =~ [[:space:]] || ! $TARGET_ARCHS =~ (^|[[:space:]])$ARCH([[:space:]]|$) ]]; then
  echo ""
  echo ">>> WRONG ARCHITECTURE \"$ARCH\""
  exit 1
fi

if [ -z "$2" ]; then
  INSTALL_DEPS=1
else
  if [ "$2" == "full" ]; then
      INSTALL_DEPS=1
  elif [ "$2" == "build" ]; then
      INSTALL_DEPS=0
  else
      echo ">>> WRONG mode use full or build"
      exit 1
  fi
fi

if [ -z "$BUILD_FORKS" ]; then
  BUILD_FORKS="-j4"
  export BUILD_FORKS
fi

echo ""
echo ">>> ARCHITECTURE \"$ARCH\""

if [ $ARCH == "ubuntu" ]; then
    BOOST_ROOT=${HOME}/opt/boost_1_66_0
    BINARYEN_BIN=${HOME}/opt/binaryen/bin
    OPENSSL_ROOT_DIR=/usr/local/opt/openssl
    OPENSSL_LIBRARIES=/usr/local/opt/openssl/lib
    WASM_LLVM_CONFIG=${HOME}/opt/wasm/bin/llvm-config
    WASM_LLVM=${HOME}/opt/wasm/
    ROCKSDB_ROOT=${HOME}/opt/rocksdb
    BEAST_ROOT=${HOME}/opt/beast
    BEAST_INCLUDE=${BEAST_ROOT}/include
    export BOOST_ROOT BINARYEN_BIN OPENSSL_ROOT_DIR OPENSSL_LIBRARIES WASM_LLVM_CONFIG ROCKSDB_ROOT BEAST_ROOT BEAST_INCLUDE WASM_LLVM

    PROTOBUF_SRC_ROOT_FOLDER=${HOME}/opt/protobuf/
    PROTOBUF_LIBRARY=${PROTOBUF_SRC_ROOT_FOLDER}/lib/
    PROTOBUF_IMPORT_DIRS=${PROTOBUF_SRC_ROOT_FOLDER}/include/
    export PROTOBUF_SRC_ROOT_FOLDER PROTOBUF_LIBRARY PROTOBUF_IMPORT_DIRS

    ASN1_ROOT_FOLDER=${HOME}/opt/asn1c/
    export ASN1_ROOT_FOLDER
    
    BOTAN_ROOT=${HOME}/opt/botan/
    export BOTAN_ROOT

    ROCKSDB_ROOT=${HOME}/opt/rocksdb/
    export ROCKSDB_ROOT

    LIBRDF_ROOT=${HOME}/opt/librdf/
    export LIBRDF_ROOT

    JSON_ROOT=${HOME}/opt/nlohmann/
    export JSON_ROOT

    CHAI_SCRIPT_ROOT=${HOME}/opt/ChaiScript/
    export CHAI_SCRIPT_ROOT

    LIBWAVM_ROOT=${HOME}/opt/wavm/
    export LIBWAVM_ROOT

    LIBWALLY_ROOT=${HOME}/opt/libwally-core/
    export LIBWALLY_ROOT
fi

if [ $ARCH == "darwin" ]; then
    OPENSSL_ROOT_DIR=/usr/local/opt/openssl
    OPENSSL_LIBRARIES=/usr/local/opt/openssl/lib
    BINARYEN_BIN=/usr/local/binaryen/bin/
    WASM_LLVM_CONFIG=/usr/local/wasm/bin/llvm-config
fi

# Debug flags
COMPILE_KETO=1
COMPILE_CONTRACTS=1

# Define default arguments.
CMAKE_BUILD_TYPE=RelWithDebugInfo

# Install dependencies
if [ ${INSTALL_DEPS} == "1" ]; then

  echo ">> Install dependencies"
  . ${WORK_DIR}/scripts/dependencies/install_dependencies.sh

fi


CXX_COMPILER=clang++-4.0
C_COMPILER=clang-4.0

if [ $ARCH == "darwin" ]; then
  CXX_COMPILER=clang++
  C_COMPILER=clang
fi

# Create the build dir
cd ${WORK_DIR}
mkdir -p ${BUILD_DEP_DIR}
cd ${BUILD_DEP_DIR}

# Build KETO 
#cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} -DCMAKE_C_COMPILER=${C_COMPILER} -DWASM_LLVM_CONFIG=${WASM_LLVM_CONFIG} -DWASM_LLVM=${WASM_LLVM} -DBINARYEN_BIN=${BINARYEN_BIN} -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR} -DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES} --build ../src/programs/tools/
cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} -DCMAKE_C_COMPILER=${C_COMPILER} -DWASM_LLVM_CONFIG=${WASM_LLVM_CONFIG} -DWASM_LLVM=${WASM_LLVM} -DBINARYEN_BIN=${BINARYEN_BIN} -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR} -DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES} ..
make ${BUILD_FORKS} install


# Create the build dir
cd ${WORK_DIR}
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# Build KETO 
#cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} -DCMAKE_C_COMPILER=${C_COMPILER} -DWASM_LLVM_CONFIG=${WASM_LLVM_CONFIG} -DWASM_LLVM=${WASM_LLVM} -DBINARYEN_BIN=${BINARYEN_BIN} -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR} -DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES} --build ../src/programs/tools/
cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} -DCMAKE_C_COMPILER=${C_COMPILER} -DWASM_LLVM_CONFIG=${WASM_LLVM_CONFIG} -DWASM_LLVM=${WASM_LLVM} -DBINARYEN_BIN=${BINARYEN_BIN} -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR} -DOPENSSL_LIBRARIES=${OPENSSL_LIBRARIES} ..
make ${BUILD_FORKS} install

