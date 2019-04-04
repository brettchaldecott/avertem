#!/usr/bin/env bash
# Install dependencies script

if [ $ARCH == "ubuntu" ]; then
    # install dev toolkit
    sudo apt-get update
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
    sudo apt-get -y install clang-4.0 lldb-4.0 libclang-4.0-dev cmake make \
                         libbz2-dev libssl-dev libgmp3-dev \
                         autotools-dev build-essential \
                         libbz2-dev libicu-dev python-dev \
                         autoconf libtool git curl \
                         libgflags-dev libsnappy-dev \
                         zlib1g-dev liblz4-dev libzstd-dev \
                         bison libbison-dev flex libfl-dev \
                         gtk-doc-tools libxml2-dev libdb-dev \
                         libpcre3 libpcre3-dev
    OPENSSL_ROOT_DIR=/usr/local/opt/openssl
    OPENSSL_LIBRARIES=/usr/local/opt/openssl/lib

    # install boost
    cd ${TEMP_DIR}
    export BOOST_ROOT=${HOME}/opt/boost_1_66_0
    curl -L https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2 > boost_1.66.0.tar.bz2
    tar xvf boost_1.66.0.tar.bz2
    cd boost_1_66_0/
    ./bootstrap.sh "--prefix=$BOOST_ROOT"
    ./b2 install
    rm -rf ${TEMP_DIR}/boost_1_66_0/

    # install secp256k1-zkp (Cryptonomex branch)
    #cd ${TEMP_DIR}
    #git clone https://github.com/cryptonomex/secp256k1-zkp.git
    #cd secp256k1-zkp
    #./autogen.sh
    #./configure
    #make
    #sudo make install
    #rm -rf cd ${TEMP_DIR}/secp256k1-zkp
    
    # install binaryen
    cd ${TEMP_DIR}
    git clone https://github.com/WebAssembly/binaryen
    cd binaryen
    git checkout tags/1.37.14
    cmake . && make
    mkdir -p ${HOME}/opt/binaryen/
    cp -rf ${TEMP_DIR}/binaryen/bin ${HOME}/opt/binaryen/.
    rm -rf ${TEMP_DIR}/binaryen
    BINARYEN_BIN=${HOME}/opt/binaryen/bin
    
    # install rocksdb
    cd ${TEMP_DIR}
    git clone https://github.com/facebook/rocksdb.git
    cd rocksdb
    mkdir -p ${HOME}/opt/rocksdb/
    #EXTRA_CFLAGS=-fPIC EXTRA_CXXFLAGS=-fPIC PORTABLE=1 make static_lib
    INSTALL_PATH=${HOME}/opt/rocksdb/ EXTRA_CFLAGS=-fPIC EXTRA_CXXFLAGS=-fPIC PORTABLE=1 make static_lib
    INSTALL_PATH=${HOME}/opt/rocksdb/ EXTRA_CFLAGS=-fPIC EXTRA_CXXFLAGS=-fPIC PORTABLE=1 make install
    #mkdir -p ${HOME}/opt/rocksdb/
    #mkdir -p ${HOME}/opt/rocksdb/lib
    #mkdir -p ${HOME}/opt/rocksdb/lib
    #mv ${TEMP_DIR}/rocksdb/librocksdb.a ${HOME}/opt/rocksdb/lib/
    #cp -rf ${TEMP_DIR}/rocksdb/include ${HOME}/opt/rocksdb/include
    cd ${HOME}
    rm -rf ${TEMP_DIR}/rocksdb

    # install beast
    cd ${HOME}/opt
    git clone https://github.com/boostorg/beast.git

    # install protobuf
    PROTOBUF_VERSION=3.5.1
    cd ${TEMP_DIR}
    wget https://github.com/google/protobuf/releases/download/v3.5.1/protobuf-all-${PROTOBUF_VERSION}.tar.gz
    tar -zxvf protobuf-all-${PROTOBUF_VERSION}.tar.gz
    mkdir -p ${HOME}/opt/protobuf
    cd ${TEMP_DIR}/protobuf-${PROTOBUF_VERSION}/
    ./configure "CFLAGS=-fPIC" "CXXFLAGS=-fPIC" --prefix ${HOME}/opt/protobuf --enable-shared=no 
    make
    make install
    cd ${HOME}
    rm -rf ${TEMP_DIR}/protobuf-${PROTOBUF_VERSION}
    
    # asn1 required for serialization of transaction and blockchain formats
    cd ${TEMP_DIR}
    git clone https://github.com/vlm/asn1c.git
    mkdir -p ${HOME}/opt/asn1c
    cd ${TEMP_DIR}/asn1c
    test -f configure || autoreconf -iv
    ./configure --prefix ${HOME}/opt/asn1c
    make
    make install
    cd ${HOME}
    rm -rf ${TEMP_DIR}/asn1c

    # botan required for encryption, hashing and 
    cd ${TEMP_DIR}
    git clone https://github.com/randombit/botan.git
    mkdir -p ${HOME}/opt/botan
    cd ${TEMP_DIR}/botan
    ./configure.py --cxxflags=-fPIC --prefix=${HOME}/opt/botan --with-openssl --disable-shared-library
    make
    make install
    cd ${HOME}
    rm -rf ${TEMP_DIR}/botan
    
    # rdf libraries 
    mkdir -p ${HOME}/opt/librdf
    cd ${TEMP_DIR}
    git clone git://github.com/dajobe/raptor.git
    cd ${TEMP_DIR}/raptor
    CFLAGS=-fPIC CPPFLAGS=-fPIC ./autogen.sh --prefix=${HOME}/opt/librdf --enable-shared=no
    make
    make install
    cd ${HOME}
    rm -rf ${TEMP_DIR}/raptor
    
    cd ${TEMP_DIR}
    git clone git://github.com/dajobe/rasqal.git
    cd ${TEMP_DIR}/rasqal
    CFLAGS=-fPIC CPPFLAGS=-fPIC PKG_CONFIG_PATH=${HOME}/opt/librdf/lib/pkgconfig ./autogen.sh --prefix=${HOME}/opt/librdf --enable-shared=no
    make
    make install
    cd ${HOME}
    rm -rf ${TEMP_DIR}/rasqal
    
    cd ${TEMP_DIR}
    git clone git://github.com/dajobe/librdf.git
    cd ${TEMP_DIR}/librdf
    CFLAGS=-fPIC CPPFLAGS=-fPIC PKG_CONFIG_PATH=${HOME}/opt/librdf/lib/pkgconfig ./autogen.sh --prefix=${HOME}/opt/librdf --enable-shared=no --with-bdb
    make
    make install
    cd ${HOME}
    rm -rf ${TEMP_DIR}/librdf

    # json parsing
    cd ${TEMP_DIR}
    git clone https://github.com/nlohmann/json.git
    cd ${TEMP_DIR}/json
    mkdir -p ${HOME}/opt/nlohmann/include
    cp -rf single_include/nlohmann ${HOME}/opt/nlohmann/include
    cd ${HOME}
    rm -rf ${TEMP_DIR}/json

    # build llvm with wasm build target:
    cd ${TEMP_DIR}
    mkdir wasm-compiler
    cd wasm-compiler
    git clone --depth 1 --single-branch --branch release_60 https://github.com/llvm-mirror/llvm.git
    cd llvm/tools
    git clone --depth 1 --single-branch --branch release_60 https://github.com/llvm-mirror/clang.git
    cd ..
    mkdir build
    cd build
    cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=${HOME}/opt/wasm \
        -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly -DCMAKE_BUILD_TYPE=Release ../
    make ${BUILD_FORKS} install
    rm -rf ${TEMP_DIR}/wasm-compiler
    WASM_LLVM_CONFIG=${HOME}/opt/wasm/bin/llvm-config
    WASM_LLVM=${HOME}/opt/wasm/
    
    # build wavm
    cd ${TEMP_DIR}
    git clone https://github.com/burntjam/WAVM.git
    mkdir -p ${TEMP_DIR}/WAVM/cmake
    cd ${TEMP_DIR}/WAVM/cmake
    cmake .. -DCMAKE_BUILD_TYPE=RELEASE -DLLVM_DIR=${WASM_LLVM}
    make
    mkdir -p ${HOME}/opt/wavm/lib
    mkdir -p ${HOME}/opt/wavm/include
    cp ${TEMP_DIR}/WAVM/cmake/lib/* ${HOME}/opt/wavm/lib/.
    cp -rf ${TEMP_DIR}/WAVM/Include/* ${HOME}/opt/wavm/include/.
    cd ${HOME}
    rm -rf ${TEMP_DIR}/WAVM

    # temp directory
    cd ${TEMP_DIR}
    git clone https://github.com/ElementsProject/libwally-core.git
    cd ${TEMP_DIR}/libwally-core
    ./tools/autogen.sh
    ./configure "CFLAGS=-fPIC" "CXXFLAGS=-fPIC" --disable-shared --prefix=${HOME}/opt/libwally-core/
    make
    make install
    
    # install beast
    cd ${HOME}/opt
    git clone https://github.com/ChaiScript/ChaiScript.git
    cd ${HOME}/opt/ChaiScript && git checkout v6.1.0



    cd ${HOME}

fi

if [ $ARCH == "darwin" ]; then
    DEPS="git automake libtool boost openssl llvm@4 gmp wget cmake gettext"
    brew update
    brew install --force $DEPS
    brew unlink $DEPS && brew link --force $DEPS
    # LLVM_DIR=/usr/local/Cellar/llvm/4.0.1/lib/cmake/llvm

    # install secp256k1-zkp (Cryptonomex branch)
    cd ${TEMP_DIR}
    git clone https://github.com/cryptonomex/secp256k1-zkp.git
    cd secp256k1-zkp
    ./autogen.sh
    ./configure
    make
    sudo make install
    sudo rm -rf ${TEMP_DIR}/secp256k1-zkp

    # Install binaryen v1.37.14:
    cd ${TEMP_DIR}
    git clone https://github.com/WebAssembly/binaryen
    cd binaryen
    git checkout tags/1.37.14
    cmake . && make
    sudo mkdir /usr/local/binaryen
    sudo mv ${TEMP_DIR}/binaryen/bin /usr/local/binaryen
    sudo ln -s /usr/local/binaryen/bin/* /usr/local
    sudo rm -rf ${TEMP_DIR}/binaryen
    BINARYEN_BIN=/usr/local/binaryen/bin/

    # Build LLVM and clang for WASM:
    cd ${TEMP_DIR}
    mkdir wasm-compiler
    cd wasm-compiler
    git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
    cd llvm/tools
    git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
    cd ..
    mkdir build
    cd build
    sudo cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=/usr/local/wasm -DLLVM_TARGETS_TO_BUILD= -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly -DCMAKE_BUILD_TYPE=Release ../
    sudo make ${BUILD_FORKS} install
    sudo rm -rf ${TEMP_DIR}/wasm-compiler
    WASM_LLVM_CONFIG=/usr/local/wasm/bin/llvm-config

fi
