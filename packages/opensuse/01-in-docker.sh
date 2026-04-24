#!/bin/bash

set -xeuo pipefail

zypper in -y git rpm-build rpmdevtools sudo
rpmdev-setuptree

./packages/opensuse/buildrpm.sh
