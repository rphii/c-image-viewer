#!/bin/sh

dir="$(dirname ${BASH_SOURCE})"

for f in $dir/*.vert $dir/*.frag; do
    xxd -i "${f}"
done > $dir/shaders_generated.h

