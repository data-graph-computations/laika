#!/usr/bin/env bash
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get update
apt-get install -y build-essential openssl htop curl wget make vim git gitk gcc-5 g++-5 linux-tools-common linux-tools-generic keychain mpich libmpich-dev mosh

pushd /usr/bin
rm g++
ln -s g++-5 g++
popd
