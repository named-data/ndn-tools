#!/usr/bin/env bash
set -x
set -e

rm -rf $HOME/.ndn
./build/unit-tests -l test_suite
