#!/usr/bin/env bash

# Fail on first error, on undefined variables, and on failures in a pipeline.
set -euo pipefail

# Print each executed line.
set -x

wget -O /tmp/mesh_inputs.tar.gz https://www.dropbox.com/s/zhzzhfc5kx31lpi/mesh_inputs.tar.gz?dl=1

mkdir -p /laika/input_data

tar -xf /tmp/mesh_inputs.tar.gz --directory /laika/input_data

rm /tmp/mesh_inputs.tar.gz
