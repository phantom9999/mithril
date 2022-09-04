FROM centos:7

# install base deps
RUN yum install -y centos-release-scl-rh curl zip unzip tar lapack-devel blas-devel openssl-devel;  \
    yum clean all; \
    rm -rf /var/cache/yum/*


# install devtoolset
RUN yum install -y wget devtoolset-11-gcc-c++ devtoolset-11-make rh-git227-git rh-python38-python rh-python38-python-pip; \
    yum clean all; \
    rm -rf /var/cache/yum/*


# use devtoolset
SHELL [ "/usr/bin/scl", "enable", "devtoolset-11", "rh-git227", "rh-python38" ]


# install cmake
RUN wget https://github.com/Kitware/CMake/releases/download/v3.23.2/cmake-3.23.2.tar.gz && \
    tar -xf cmake-3.23.2.tar.gz && cd cmake-3.23.2 && \
    ./bootstrap --parallel=16 && make -j16 && make install && \
    cd .. && rm -rf cmake-3.23.2.tar.gz cmake-3.23.2


# install faiss
RUN wget https://github.com/facebookresearch/faiss/archive/refs/tags/v1.7.2.tar.gz && \
    tar -xf v1.7.2.tar.gz && cd faiss-1.7.2 && \
    cmake -DFAISS_OPT_LEVEL=avx2 -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF -DBUILD_TESTING=OFF . && \
    make -j16 && make install


# install tonan for other deps
RUN pip install conan
