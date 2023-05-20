FROM centos:7

RUN yum install -y centos-release-scl-rh curl zip unzip tar;

RUN yum install -y wget devtoolset-11-gcc-c++ devtoolset-11-make rh-git227-git rh-python38-python; \
    yum clean all; \
    rm -rf /var/cache/yum/*

#RUN echo "source /opt/rh/devtoolset-11/enable" >> /opt/dev.sh; \
#   echo "source /opt/rh/rh-git227/enable" >> /opt/dev.sh; \
#   echo "source /opt/rh/rh-python38/enable" >> /opt/dev.sh;

#ENV BASH_ENV=/opt/dev.sh \
#    ENV=/opt/dev.sh \
#    PROMPT_COMMAND=". /opt/dev.sh"

SHELL [ "/usr/bin/scl", "enable", "devtoolset-11", "rh-git227", "rh-python38" ]
