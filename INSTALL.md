# ndn-tools Build Instructions

This document describes how to build and install ndn-tools.

## Prerequisites

- Install the [ndn-cxx](https://named-data.net/doc/ndn-cxx/current/) library and its prerequisites.
  Please see [Getting Started with ndn-cxx](https://named-data.net/doc/ndn-cxx/current/INSTALL.html)
  for instructions.
  All operating systems and compilers supported by ndn-cxx are also supported by ndn-tools.

  *Note*: If you have installed ndn-cxx from a binary package, please make sure the development
  headers are installed (e.g., if using Ubuntu PPA, the `libndn-cxx-dev` package is needed).

- `libpcap`

  Comes with the base system on macOS.

  On Ubuntu:

      sudo apt install libpcap-dev

  On CentOS and Fedora:

      sudo dnf config-manager --enable powertools  # CentOS only
      sudo dnf install libpcap-devel

## Build Steps

To configure, compile, and install ndn-tools, type the following commands
in ndn-tools source directory:

    ./waf configure
    ./waf
    sudo ./waf install

To uninstall:

    sudo ./waf uninstall
