#!/bin/bash

rm -Rfv include
rm -Rfv src
rm -v CMakeLists.txt

cp -av /home/vitaly/_git_repos/libOPNMIDI/include .
cp -av /home/vitaly/_git_repos/libOPNMIDI/src .
cp -v /home/vitaly/_git_repos/libOPNMIDI/CMakeLists.txt .

cp -v /home/vitaly/_git_repos/libOPNMIDI/fm_banks/xg.wopn ../assets

read -n 1

