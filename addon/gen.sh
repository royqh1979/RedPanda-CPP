#!/bin/bash

set -euo pipefail

function gen-theme() {
  local file="$1"
  local bn="$(basename $file)"
  local out="RedPandaIDE/resources/themes/${bn%.tl}.lua"
  echo -e "\033[1;33mChecking $file\033[0m"
  tl check --include-dir addon --global-env-def defs/theme --quiet "$file"
  echo -e "\033[1;32mCompiling $file\033[0m"
  tl gen --include-dir addon --global-env-def defs/theme --gen-compat off --gen-target 5.4 -o "$out" "$file"
}

for file in addon/theme/*.tl; do
  gen-theme "$file"
done

function gen-compiler-hint() {
  local file="$1"
  local out="$2"
  echo -e "\033[1;33mChecking $file\033[0m"
  tl check --include-dir addon --global-env-def defs/compiler_hint --quiet "$file"
  echo -e "\033[1;32mCompiling $file\033[0m"
  tl gen --include-dir addon --global-env-def defs/compiler_hint --gen-compat off --gen-target 5.4 -o "$out" "$file"
}

gen-compiler-hint addon/compiler_hint/windows_llvm.tl packages/msys/compiler_hint.lua
gen-compiler-hint addon/compiler_hint/archlinux.tl packages/archlinux/compiler_hint.lua
gen-compiler-hint addon/compiler_hint/debian.tl packages/debian/compiler_hint.lua
