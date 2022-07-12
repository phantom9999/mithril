FROM ubuntu:22.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        automake \
        build-essential \
        ca-certificates \
        curl \
        git \
        libcurl3-dev \
        libfreetype6-dev \
        libpng-dev \
        libtool \
        libzmq3-dev \
        mlocate \
        pkg-config \
        python3-dev \
        software-properties-common \
        swig \
        unzip \
        wget \
        zip \
        zlib1g-dev \
        python3-distutils \
        && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/bazelbuild/bazelisk/releases/download/v1.11.0/bazelisk-linux-amd64 -O /usr/bin/bazelisk && \
    chmod +x /usr/bin/bazelisk && \
    /usr/bin/bazelisk --version



