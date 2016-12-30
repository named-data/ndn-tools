#!/usr/bin/env bash
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

set -x

# Cleanup
sudo ./waf -j1 --color=yes distclean

if [[ $JOB_NAME != *"code-coverage" && $JOB_NAME != *"limited-build" ]]; then
  # Configure/build in optimized mode with tests
  ./waf -j1 --color=yes configure --with-tests
  ./waf -j1 --color=yes build

  # Cleanup
  sudo ./waf -j1 --color=yes distclean

  # Configure/build in optimized mode without tests
  ./waf -j1 --color=yes configure
  ./waf -j1 --color=yes build

  # Cleanup
  sudo ./waf -j1 --color=yes distclean
fi

# Configure/build in debug mode with tests
if [[ $JOB_NAME == *"code-coverage" ]]; then
    COVERAGE="--with-coverage"
elif ! has OSX-10.9 $NODE_LABELS && ! has OSX-10.11 $NODE_LABELS; then
    ASAN="--with-sanitizer=address"
fi
./waf -j1 --color=yes configure --debug --with-tests $COVERAGE $ASAN
./waf -j1 --color=yes build

# (tests will be run against debug version)

# Install
sudo ./waf -j1 --color=yes install
