#!/bin/bash

docker run --rm -u $(id -u ${USER}) -v $(pwd):/home/builder/ -v /tmp/:/tmp/ bertos-meson-build $@
