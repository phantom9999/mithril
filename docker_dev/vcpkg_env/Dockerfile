FROM centos:7

RUN yum install -y centos-release-scl-rh curl zip unzip tar;

RUN yum install -y wget devtoolset-11-gcc-c++ devtoolset-11-make rh-git227-git; \
    yum clean all; \
    rm -rf /var/cache/yum/* 

RUN echo "source /opt/rh/devtoolset-11/enable" >> /opt/dev.sh; \
   echo "source /opt/rh/rh-git227/enable" >> /opt/dev.sh;

ENV BASH_ENV=/opt/dev.sh \
    ENV=/opt/dev.sh \
    PROMPT_COMMAND=". /opt/dev.sh"

RUN source /opt/dev.sh; \
    cd /opt; \
    wget https://github.com/microsoft/vcpkg/archive/refs/tags/2022.02.23.tar.gz; \
    tar -xf 2022.02.23.tar.gz; \
    ln -s /opt/vcpkg-2022.02.23 /opt/vcpkg_home; \
    rm 2022.02.23.tar.gz;

RUN source /opt/dev.sh; \
    whereis git; \
    cd /opt/vcpkg_home; \
    ./bootstrap-vcpkg.sh -disableMetrics;

ENV PATH=/opt/vcpkg_home:$PATH

RUN vcpkg install gflags glog

