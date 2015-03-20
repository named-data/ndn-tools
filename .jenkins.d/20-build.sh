#!/usr/bin/env bash
set -x
set -e

# Cleanup
sudo ./waf -j1 --color=yes distclean

# Configure/build in release mode
./waf -j1 --color=yes configure
./waf -j1 --color=yes build

# Cleanup
sudo ./waf -j1 --color=yes distclean

# Configure/build in debug mode
./waf -j1 --color=yes configure --debug
./waf -j1 --color=yes build
