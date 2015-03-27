# ndn-tools Build Instructions

This document describes how to build and install ndn-tools.

## Prerequisites

Building ndn-tools requires [ndn-cxx](http://named-data.net/doc/ndn-cxx/current/)
to be installed.  
Please see [Getting Started with ndn-cxx](http://named-data.net/doc/ndn-cxx/current/INSTALL.html)
on how to install ndn-cxx.  
Note: if you have installed ndn-cxx from a binary package, please make sure development headers
are installed (if using Ubuntu PPA, `ndn-cxx-dev` package is needed).

Any operating system and compiler supported by ndn-cxx are supported by ndn-tools.

## Build Steps

Waf meta build system is used by ndn-tools.

To configure, compile, and install ndn-tools, type the following commands
in ndn-tools source directory:

    ./waf configure
    ./waf
    sudo ./waf install

To uninstall ndn-tools:

    sudo ./waf uninstall
