FROM centos:7


RUN yum install -y centos-release-scl-rh

# 基本开发环境
#RUN dnf install -y gcc-c++ cmake make git sudo; \
#    dnf clean all; \
#    rm -rf /var/cache/dnf/*

RUN yum install -y devtoolset-11-gcc-c++ git sudo; \
    yum clean all; \
    rm -rf /var/cache/yum/* 

# python环境
RUN yum install -y python3 python3-pip python3-pyOpenSSL python3-cryptography \
    less libXext libXrender libXtst libXi freetype; \
    yum clean all; \
    rm -rf /var/cache/yum/*

# 创建用户及目录
RUN useradd -b /home -m -s /bin/bash work
WORKDIR /home/work
RUN usermod -a -G wheel work
RUN echo "work:work"|chpasswd 
USER work

ENV LANG=en_US.utf-8

# 安装projector和conan
RUN python3 -m pip install -U pip --user; pip3 cache purge;
RUN pip3 install projector-installer conan --user; pip3 cache purge;
RUN mkdir -p .projector/apps .projector/cache .projector/configs
RUN .local/bin/projector autoinstall --config-name CLion --ide-name "CLion 2021.3.3" --port 9999; \
    rm .projector/cache/*

#RUN echo 'source /opt/rh/devtoolset-11/enable' >> ~/.bashrc

EXPOSE 9999

ADD run.sh /home/work/run.sh

ENTRYPOINT [ "/home/work/run.sh" ]

#RUN dnf install -y boost-devel gperftools-devel snappy-devel openssl-devel gflags-devel protobuf-devel protobuf-compiler leveldb-devel cmake gcc-c++ make wget; \
#        dnf clean all; \
#        rm -rf /var/cache/dnf/*

#RUN /usr/bin/ssh-keygen -A
#EXPOSE 22
#CMD ["/usr/sbin/sshd", "-D"]