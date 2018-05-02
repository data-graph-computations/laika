#!/usr/bin/env bash

# Fail on first error, on undefined variables, and on failures in a pipeline.
set -euo pipefail

# Print each executed line.
set -x

apt-get update

apt-get install -y \
    build-essential openssl htop curl wget make vim git \
    linux-tools-common linux-tools-generic keychain mosh \
    python python3.6
