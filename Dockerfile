FROM ubuntu:22.04

# refresh packages
RUN apt update

# git
RUN apt install -y git

# build essential
RUN apt install -y build-essential

# clang
RUN apt install -y clang libclang-dev

# change into home directory
WORKDIR "/root"

# project submission
RUN git clone https://github.com/nathanvercaemert/nhvercae-766-course-project.git