FROM ubuntu:18.04
RUN apt-get update
RUN apt-get install -y python3 python3-pip bear time
RUN pip3 install requests
RUN apt-get install build-essential
RUN dpkg --add-architecture i386
RUN apt-get update
RUN apt-get install -y libc6-i386 gawk