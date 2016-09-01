#!/usr/bin/env bash
set -x
set -e

rm -rf ~/.ndn

ASAN_OPTIONS="color=always"
ASAN_OPTIONS+=":detect_leaks=false"
ASAN_OPTIONS+=":detect_stack_use_after_return=true"
ASAN_OPTIONS+=":check_initialization_order=true"
ASAN_OPTIONS+=":strict_init_order=true"
ASAN_OPTIONS+=":detect_invalid_pointer_pairs=1"
ASAN_OPTIONS+=":detect_container_overflow=false"
ASAN_OPTIONS+=":strict_string_checks=true"
ASAN_OPTIONS+=":strip_path_prefix=${PWD}/"
export ASAN_OPTIONS

./build/unit-tests -l test_suite
