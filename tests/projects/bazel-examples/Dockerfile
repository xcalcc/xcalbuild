FROM ubuntu:18.04
RUN apt-get update
RUN apt-get install -y python3 python3-pip bear time
RUN pip3 install requests
RUN echo 'deb [trusted=yes] https://storage.googleapis.com/bazel-apt stable jdk1.8' >> /etc/apt/sources.list
RUN apt-get update && apt-get install -y build-essential cmake bazel strace
