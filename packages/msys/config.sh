#!/bin/bash

ASTYLE_VERSION_TAG="3.6.9"

source version.inc
[[ -n "${APP_VERSION_SUFFIX}" ]] && APP_VERSION="${APP_VERSION}${APP_VERSION_SUFFIX}"
