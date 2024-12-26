#!/bin/bash

input_icon="../RedPandaIDE/images/devcpp.png"
output_dir="RedPandaIDE.iconset"

mkdir $output_dir

sips -z 16 16     $input_icon --out "${output_dir}/icon_16x16.png"
sips -z 32 32     $input_icon --out "${output_dir}/icon_16x16@2x.png"
sips -z 32 32     $input_icon --out "${output_dir}/icon_32x32.png"
sips -z 64 64     $input_icon --out "${output_dir}/icon_32x32@2x.png"
sips -z 128 128   $input_icon --out "${output_dir}/icon_128x128.png"
sips -z 256 256   $input_icon --out "${output_dir}/icon_128x128@2x.png"
sips -z 256 256   $input_icon --out "${output_dir}/icon_256x256.png"
sips -z 512 512   $input_icon --out "${output_dir}/icon_256x256@2x.png"

iconutil -c icns $output_dir

rm -R $output_dir
