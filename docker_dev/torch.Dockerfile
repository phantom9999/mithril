FROM rockylinux:8

# gcc
RUN yum install -y gcc-c++ cmake wget automake autoconf libtool unzip git \
    boost-devel openssl-devel zlib-devel libcurl-devel && \
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

RUN git clone --depth 1 --branch v1.47.0 https://github.com/grpc/grpc.git && \
    cd grpc && git submodule update --init --depth 1 && \
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DgRPC_BUILD_TESTS=OFF  \
    -DgRPC_ZLIB_PROVIDER=package -DgRPC_SSL_PROVIDER=package . && \
    make -j16 && make install && \
    cd .. && rm -rf grpc

RUN wget https://github.com/libuv/libuv/archive/refs/tags/v1.44.2.tar.gz && \
    tar -xf v1.44.2.tar.gz && cd libuv-1.44.2/ && \
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo . && \
    make -j16 && make install && \
    cd .. && rm -rf libuv-1.44.2/ v1.44.2.tar.gz

RUN git clone --depth 1 --branch v1.1.0 https://github.com/jupp0r/prometheus-cpp.git && \
    cd prometheus-cpp && git submodule update --init --depth 1 && \
    mkdir build && cd build && \
    cmake -DENABLE_TESTING=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && \
    make -j16 && make install && cd ../../ && rm -rf prometheus-cpp

#RUN git clone --depth 1 --branch v1.13.1 https://github.com/pytorch/pytorch && \
#    cd pytorch && git submodule update --init --recursive && \
#    mkdir build && cd build && \
#    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_BINARY=ON -DBUILD_CUSTOM_PROTOBUF=OFF -DBUILD_PYTHON=OFF \
#    -DBUILD_SHARED_LIBS=OFF -DUSE_CUDA=OFF -DUSE_GLOG=ON -DUSE_GFLAGS=ON -DUSE_NUMPY=OFF -DBUILD_LIBTORCH_CPU_WITH_DEBUG=ON \
#    -DUSE_NATIVE_ARCH=ON .. && make -j16 && make install && \
#    cd ../../ && rm -rf pytorch

RUN cd /opt && wget https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.13.1%2Bcpu.zip && \
    unzip libtorch-cxx11-abi-shared-with-deps-1.13.1+cpu.zip && \
    rm -rf libtorch-cxx11-abi-shared-with-deps-1.13.1+cpu.zip

RUN git clone --depth 1 --branch v1.14.1 https://github.com/Microsoft/onnxruntime.git && \
    cd onnxruntime && git submodule update --init --depth 1 && \
    sh build.sh --config RelWithDebInfo --build_shared_lib --parallel --allow_running_as_root && \
    cd build/Linux/RelWithDebInfo/ && make install && cd ../../../../ && \
    rm -rf onnxruntime

ENV Torch_ROOT=/opt/libtorch

