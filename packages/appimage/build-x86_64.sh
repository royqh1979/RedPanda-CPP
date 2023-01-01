#!/bin/bash

set -xe

arch=x86_64

docker run --rm -v $PWD:/build/RedPanda-CPP -e CARCH=$arch redpanda-builder-$arch /build/RedPanda-CPP/packages/appimage/01-in-docker.sh
