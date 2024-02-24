#!/bin/bash

set -e

execute_clean() {
    rm -rf build
}

execute_cmake() {
    execute_clean
    mkdir -p build
    cd build
    
    cmake ..
}


if [ "$1" = "-c" ] || [ "$1" = "clean" ]; then
    echo "clean..."
    execute_clean
else
    execute_cmake
    make -j$(nproc)
fi
