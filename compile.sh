#!/bin/bash

echo "Using docker image..."
./docker.sh make -j8 -f Makefile.docker "$@" || exit 1
