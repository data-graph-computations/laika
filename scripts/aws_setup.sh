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

# The below package is specific to AWS instances.
# If using a different type of machine, it may not be necessary,
# or a different package may be necessary.
apt-get install -y linux-tools-aws

# One final, manual configuration step is required:
# disabling transparent huge pages.
echo ''
echo 'Please ensure that transparent huge pages (THP) are disabled on your system.'
echo 'This setting may not persist across machine restarts, so the setup will not'
echo 'be able to take care of it for you.'
echo ''
echo 'You can disable them by using the following commands:'
echo '  sudo su'
echo '  echo never > /sys/kernel/mm/transparent_hugepage/enabled'
