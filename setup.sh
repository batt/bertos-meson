#!/bin/bash

docker image build -f Dockerfile.gcc  -t "bertos-meson-build" .
