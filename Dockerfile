FROM ubuntu:22.04

# refresh packages
RUN apt update # remove this
RUN apt update

# bash
RUN bash

# git
RUN apt install -y git

# curl
RUN apt install -y curl

# wget
RUN apt install -y wget

# build essential
RUN apt install -y build-essential

# spelling
RUN apt install -y aspell ispell

# ripgrep
RUN apt install -y ripgrep

# latex
RUN apt update
RUN apt install -y texlive-latex-base
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC
RUN apt install -y texlive-latex-extra
ENV DEBIAN_FRONTEND=
ENV TZ=

# change into home directory
WORKDIR "/root"

# #######
# #######
# mutagen
# #######
# #######
# homebrew
# dependency for homebrew
RUN apt update
# homebrew installation (redirect at end of command to skip prompt)
RUN /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)" </dev/null
# mutagen installation
RUN /home/linuxbrew/.linuxbrew/bin/brew install mutagen-io/mutagen/mutagen
# remove dependencies
# RUN apt remove -y build-essential

# *****
# *****
# emacs
# *****
# *****
# add apt repositories fix (can not add emacs ppa without it)
RUN apt install -y software-properties-common
# install
RUN add-apt-repository ppa:kelleyk/emacs
RUN apt update
RUN apt install -y emacs28-nativecomp

# toggle to re-clone emacs configuration
# RUN ls &&\
#     git clone https://github.com/nathanvercaemert/emacs.git
# toggle
RUN git clone https://github.com/nathanvercaemert/emacs.git

# install emacs packages
RUN ln -s ./emacs/.emacs.d
RUN ln -s ./emacs/.emacs
# run .emacs as script so that it completes and closes
# (also because otherwise my config isn't loaded)
RUN emacs --script .emacs

# 766
RUN apt update
RUN apt install -y clang libclang-dev

# # llvm
# RUN apt install -y ninja-build cmake
# RUN git clone https://github.com/llvm/llvm-project.git
# RUN mkdir llvm-project/build
# WORKDIR "/root/llvm-project"
# RUN cmake -B ./build -G Ninja ./llvm -DLLVM_ENABLE_PROJECTS="mlir;clang" -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" -DCMAKE_BUILD_TYPE=Release
# WORKDIR "/root/llvm-project/build"
# RUN ninja -j 2 install
# # maybe can remove these things?
# WORKDIR "/root"

# # graphical applications
# # the ip address is found with ipconfig in windows
# # (then add ":0" at the end)
# # mobaxterm must be running on windows
# RUN apt install -y x11-apps
# ENV DISPLAY 172.24.224.1:0

# # pip
# RUN apt install -y python3-pip

# # matplotlib
# RUN pip install matplotlib
