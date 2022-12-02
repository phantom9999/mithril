FROM rockylinux:8

WORKDIR /home/work

RUN yum install -y epel-release && \
    yum install -y gcc-c++ cmake automake autoconf libtool wget git \
     boost-devel zlib-devel hiredis-devel && \
    yum clean all

RUN wget https://github.com/gflags/gflags/archive/refs/tags/v2.2.2.tar.gz && \
    tar -xf v2.2.2.tar.gz && cd gflags-2.2.2 && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && \
    make -j4 && make install && cd ../../ && rm -rf gflags-2.2.2 v2.2.2.tar.gz

RUN wget https://github.com/google/glog/archive/refs/tags/v0.6.0.tar.gz && \
    tar -xf v0.6.0.tar.gz && cd glog-0.6.0 && mkdir build && cd build && \
    cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=OFF .. && \
    make -j4 && make install && cd ../../ && rm -rf glog-0.6.0 v0.6.0.tar.gz

RUN git clone -b v1.47.0 https://github.com/grpc/grpc.git && \
    cd grpc && git submodule init && git submodule update && \
    (cd third_party/cares/cares && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCARES_STATIC=ON -DCARES_SHARED=OFF -DCARES_STATIC_PIC=ON . && make -j4 && make install) && \
    (cd third_party/protobuf/ && ./autogen.sh && CXXFLAGS="-fPIC -O2 -g" ./configure --disable-shared && make -j4 && make install) && \
    (cd third_party/abseil-cpp/ && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo . && cmake -j4 && make install) && \
    (cd third_party/re2/  && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRE2_BUILD_TESTING=OFF . && make -j4 && make install) && \
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DgRPC_BUILD_TESTS=OFF -DgRPC_ZLIB_PROVIDER=package -DgPRC_SSL_PROVIDER=package \
    -DgRPC_PROTOBUF_PROVIDER=package -DgRPC_CARES_PROVIDER=package -DgRPC_RE2_PROVIDER=package -DgRPC_ABSL_PROVIDER=package . && \
    make -j4 && make install && cd .. && rm -rf grpc

RUN wget https://github.com/libuv/libuv/archive/refs/tags/v1.42.0.tar.gz && \
    tar -xf v1.42.0.tar.gz && cd libuv-1.42.0/ && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=OFF . && \
    make -j4 && make install && cd ../ && rm -rf libuv-1.42.0 v1.42.0.tar.gz

RUN wget https://github.com/sewenew/redis-plus-plus/archive/refs/tags/1.3.6.tar.gz && \
    tar -xf 1.3.6.tar.gz && cd redis-plus-plus-1.3.6/ && mkdir build && cd build && \
    cmake -DREDIS_PLUS_PLUS_BUILD_ASYNC=libuv -DREDIS_PLUS_PLUS_ASYNC_FUTURE=std -DREDIS_PLUS_PLUS_BUILD_SHARED=OFF -DREDIS_PLUS_PLUS_BUILD_TEST=OFF .. && \
    make -j4 && make install && cd ../../ && rm -rf redis-plus-plus-1.3.6 1.3.6.tar.gz







