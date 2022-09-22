FROM ubuntu:18.04

# change to AU mirrors.
RUN cp /etc/apt/sources.list /etc/apt/sources.list.bk && rm /etc/apt/sources.list && touch /etc/apt/sources.list && \
        echo deb "http://mirrors.aliyun.com/ubuntu/ bionic main restricted universe multiverse" >> /etc/apt/sources.list && \
        echo deb "http://mirrors.aliyun.com/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list && \
        echo deb "http://mirrors.aliyun.com/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
        echo deb "http://mirrors.aliyun.com/ubuntu/ bionic-proposed main restricted universe multiverse" >> /etc/apt/sources.list && \
        echo deb "http://mirrors.aliyun.com/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
        echo deb-src "http://mirrors.aliyun.com/ubuntu/ bionic main restricted universe multiverse" >> /etc/apt/sources.list && \
        echo deb-src "http://mirrors.aliyun.com/ubuntu/ bionic-security main restricted universe multiverse" >> /etc/apt/sources.list && \
        echo deb-src "http://mirrors.aliyun.com/ubuntu/ bionic-updates main restricted universe multiverse" >> /etc/apt/sources.list && \
        echo deb-src "http://mirrors.aliyun.com/ubuntu/ bionic-proposed main restricted universe multiverse" >> /etc/apt/sources.list && \
        echo deb-src "http://mirrors.aliyun.com/ubuntu/ bionic-backports main restricted universe multiverse" >> /etc/apt/sources.list && \
        apt-get update && apt-get upgrade --yes
RUN apt-get install -y ca-certificates
# clang/llvm 10
RUN echo 'deb [trusted=yes] http://apt.llvm.org/bionic/ llvm-toolchain-bionic-10 main' >> /etc/apt/sources.list
# cmake
RUN echo 'deb [trusted=yes] https://apt.kitware.com/ubuntu/ bionic main' >> /etc/apt/sources.list
RUN apt-get update && apt-get install -y \
    clang-10 \
    build-essential \
    cmake \
    git \
    curl zip unzip \
    python3 \
    pkg-config \
    libc6-dev-i386

ENV CC=clang-10
ENV CXX=clang++-10
#RUN git clone https://github.com/Microsoft/vcpkg.git && cd vcpkg &&./bootstrap-vcpkg.sh -disableMetrics
RUN git clone https://gitee.com/mirrors/vcpkg.git && cd vcpkg &&./bootstrap-vcpkg.sh -disableMetrics
RUN vcpkg/vcpkg --disable-metrics install boost nlohmann-json libarchive[core]
