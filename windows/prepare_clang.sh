#!/bin/bash

ORIGIN="/clang64"
TARGET="/tmp/clang64"

echo "Copying folder..."
rm -rf $TARGET
cp -a $ORIGIN $TARGET

#remove files not needed
pushd .
cd $TARGET
rm -rf bin/llvm-exegesis.exe
rm -rf bin/c-index-test.exe
rm -rf bin/obj2yaml.exe
rm -rf bin/yaml2obj.exe
rm -rf bin/ld.lld.exe
rm -rf bin/ld64.lld.exe
rm -rf bin/lld.exe
rm -rf bin/lld-link.exe
rm -rf bin/wasm-ld.exe
rm -rf bin/llvm-readelf.exe
rm -rf bin/llvm-readobj.exe
rm -rf bin/tcl86.dll
rm -rf bin/tk86.dll
rm -rf bin/llvm-objdump.exe
rm -rf bin/llvm-bitcode-strip.exe
rm -rf bin/llvm-install-name-tool.exe
rm -rf bin/llvm-objcopy.exe
rm -rf bin/llvm-strip.exe
rm -rf bin/sqlite3_analyzer.exe
rm -rf bin/sqldiff.exe
rm -rf bin/dbhash.exe
rm -rf bin/glewinfo.exe
rm -rf bin/diagtool.exe
rm -rf bin/FileCheck.exe
rm -rf bin/KillTheDoctor.exe

rm -rf lib/libclang*.a
rm -rf lib/libLLVM*.a
rm -rf lib/libtcl*.a
rm -rf lib/libtk*.a
rm -rf lib/liblldELF.a
rm -rf lib/liblldCOFF.a
rm -rf lib/liblldMachO.a
rm -rf lib/libtdbc*
rm -rf lib/tcl8
rm -rf lib/tcl8.6
rm -rf lib/tk8.6
rm -rf lib/tdbc*
rm -rf lib/terminfo

rm -rf include/clang
rm -rf include/clang-c
rm -rf include/llvm
rm -rf include/llvm-c
rm -rf include/tcl8.6
rm -rf include/tk8.6
rm -rf include/lldb

rm -rf share/doc
rm -rf share/info
rm -rf share/man
rm -rf share/sqlite
rm -rf share/locale
rm -rf share/gtk-doc
rm -rf share/terminfo

rm -rf var

find . -name "__pycache__" -exec rm -rf {} \;
popd





