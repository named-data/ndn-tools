# ndn-tools Build Instructions

This document describes how to build and install ndn-tools.

## Prerequisites

* Install the [ndn-cxx](https://github.com/named-data/ndn-cxx) library and its dependencies.
  Check out [the documentation](https://docs.named-data.net/ndn-cxx/current/INSTALL.html) for
  detailed installation instructions. All platforms supported by ndn-cxx are also supported
  by ndn-tools.

  *Note*: If you have installed ndn-cxx from a binary package, please make sure the development
  headers are installed (e.g., if using Ubuntu PPA, the `libndn-cxx-dev` package is needed).

* Install `libpcap` (except on macOS where it is provided by the base system).

  On Debian and Ubuntu:

  ```shell
  sudo apt install libpcap-dev
  ```

  On CentOS and Fedora:

  ```shell
  sudo dnf install libpcap-devel
  ```

## Build Steps

To configure, compile, and install ndn-tools, type the following commands
in ndn-tools source directory:

```shell
./waf configure
./waf
sudo ./waf install
```

To uninstall:

```shell
sudo ./waf uninstall
```
