#!/bin/bash

rm -Rfv include
rm -Rfv src
rm -Rfv cmake
rm -v CMakeLists.txt
rm -v libOPNMIDIConfig.cmake.in

cp -av ../../../../../libOPNMIDI/include .
cp -av ../../../../../libOPNMIDI/src .
cp -v ../../../../../libOPNMIDI/CMakeLists.txt .
cp -v ../../../../../libOPNMIDI/libOPNMIDIConfig.cmake.in .

mkdir -p ./cmake
cp -av ../../../../../libOPNMIDI/cmake/checks ./cmake/

cp -v ../../../../../libOPNMIDI/fm_banks/xg.wopn ../assets

echo "Press any key..."
read -n 1
