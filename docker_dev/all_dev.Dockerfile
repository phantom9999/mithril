FROM rockylinux:8

# gcc
RUN yum install -y epel-release &&  \
    yum install -y gcc-c++ cmake wget automake autoconf libtool git unzip \
    boost-devel libunwind-devel openssl-devel zlib-devel && \
    yum clean all && rm -rf /var/log/anaconda /anaconda-post.log /var/lib/yum

# protobuf
RUN wget https://github.com/protocolbuffers/protobuf/archive/refs/tags/v3.19.5.tar.gz && \
    tar -xf v3.19.5.tar.gz && \
    cd protobuf-3.19.5/ && \
    ./autogen.sh && \
    CXXFLAGS="-O2 -g" ./configure --enable-shared=no --with-pic=yes --with-zlib && \
    make -j16 && \
    make install && \
    cd .. && rm -rf protobuf-3.19.5 v3.19.5.tar.gz;

# gflags
RUN wget https://github.com/gflags/gflags/archive/v2.2.2.tar.gz && \
    tar -xf v2.2.2.tar.gz && cd gflags-2.2.2/ && \
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF . && \
    make -j16 && make install && \
    cd .. && rm -rf gflags-2.2.2/ v2.2.2.tar.gz

# glog; 依赖gflags
RUN wget https://github.com/google/glog/archive/refs/tags/v0.6.0.tar.gz && \
    tar -xf v0.6.0.tar.gz && cd glog-0.6.0 && \
    cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=OFF . && \
    make -j16 && make install && cd .. && rm -rf glog-0.6.0 v0.6.0.tar.gz

RUN git clone https://github.com/grpc/grpc.git && \
    cd grpc && git checkout v1.47.0 && \
    git submodule init && git submodule update && \
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DgRPC_BUILD_TESTS=OFF  \
    -DgRPC_ZLIB_PROVIDER=package -DgRPC_SSL_PROVIDER=package -DgRPC_PROTOBUF_PROVIDER=package . && \
    make -j16 && make install && \
    cd .. && rm -rf grpc

# leveldb
RUN wget https://github.com/google/leveldb/archive/refs/tags/1.23.tar.gz && \
    tar -xf 1.23.tar.gz && \
    cd leveldb-1.23 && \
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DLEVELDB_BUILD_TESTS=OFF -DLEVELDB_BUILD_BENCHMARKS=OFF . && \
    make -j16 && make install && cd .. && rm -rf leveldb-1.23 1.23.tar.gz

# brpc
RUN wget https://github.com/apache/brpc/archive/refs/tags/1.4.0.tar.gz && \
    tar -xf 1.4.0.tar.gz && \
    cd brpc-1.4.0 && \
    sed -i '245c    set(DYNAMIC_LIB ${GLOG_LIB} -lunwind ${DYNAMIC_LIB})' CMakeLists.txt && \
    sed -i '246c    set(BRPC_PRIVATE_LIBS "-lglog -lunwind ${BRPC_PRIVATE_LIBS}")' CMakeLists.txt && \
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_GLOG=ON . && \
    make -j16 && make install && cd .. && rm -rf 1.4.0.tar.gz brpc-1.4.0