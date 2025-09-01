#!/bin/bash

rm -Rfv include
rm -Rfv src
rm -Rfv cmake
rm -v CMakeLists.txt
rm -v libOPNMIDIConfig.cmake.in

cp -av /home/vitaly/_git_repos/libOPNMIDI/include .
cp -av /home/vitaly/_git_repos/libOPNMIDI/src .
cp -v /home/vitaly/_git_repos/libOPNMIDI/CMakeLists.txt .
cp -v /home/vitaly/_git_repos/libOPNMIDI/libOPNMIDIConfig.cmake.in .

mkdir -p ./cmake
cp -av /home/vitaly/_git_repos/libOPNMIDI/cmake/checks ./checks

cp -v /home/vitaly/_git_repos/libOPNMIDI/fm_banks/xg.wopn ../assets

echo "Press any key..."
read -n 1
