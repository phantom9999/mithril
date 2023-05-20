FROM ubuntu:22.04

# intel-mkl libcurl4-openssl-dev
RUN apt-get update && apt-get install -y --no-install-recommends \
    automake build-essential libtool pkg-config git wget python3 python3-pip cmake \
    zlib1g-dev libzmq3-dev libssl-dev libopenblas-dev liblapacke-dev libgoogle-glog-dev libgflags-dev libgrpc++-dev protobuf-compiler-grpc libabsl-dev libboost-all-dev && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/facebookresearch/faiss/archive/refs/tags/v1.7.2.tar.gz && \
    tar -xf v1.7.2.tar.gz && cd faiss-1.7.2 && \
    cmake -DFAISS_OPT_LEVEL=avx2 -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF -DBUILD_TESTING=OFF . && \
    make -j16 && make install
    
