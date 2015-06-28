#!/usr/bin/env bash
set -x
set -e

# Cleanup
sudo ./waf -j1 --color=yes distclean

# Configure/build in release mode without tests
./waf -j1 --color=yes configure
./waf -j1 --color=yes build

# Cleanup
sudo ./waf -j1 --color=yes distclean

# Configure/build in debug mode with tests
./waf -j1 --color=yes configure --with-tests --debug
./waf -j1 --color=yes build
