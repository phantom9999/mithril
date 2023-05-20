FROM rockylinux:8

# gcc
RUN yum install -y gcc-c++ cmake wget automake autoconf libtool boost-devel openssl-devel zlib-devel git && \
    yum clean all && rm -rf /var/log/anaconda /anaconda-post.log /var/lib/yum

# gflags
RUN wget https://github.com/gflags/gflags/archive/v2.2.2.tar.gz && \
    tar -xf v2.2.2.tar.gz && cd gflags-2.2.2/ && \
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF . && \
    make -j16 && make install && \
    cd .. && rm -rf gflags-2.2.2/ v2.2.2.tar.gz

# glog; 依赖gflags和libunwind
RUN wget https://github.com/google/glog/archive/refs/tags/v0.6.0.tar.gz && \
    tar -xf v0.6.0.tar.gz && cd glog-0.6.0 && \
    cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=OFF . && \
    make -j16 && make install && cd .. && rm -rf glog-0.6.0 v0.6.0.tar.gz

# grpc
RUN git clone --depth 1 --branch v1.47.0 https://github.com/grpc/grpc.git && \
    cd grpc && git submodule update --init && \
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DgRPC_BUILD_TESTS=OFF  \
    -DgRPC_ZLIB_PROVIDER=package -DgRPC_SSL_PROVIDER=package . && \
    make -j16 && make install && \
    cd .. && rm -rf grpc
