# NDN Essential Tools

![Latest version](https://img.shields.io/github/v/tag/named-data/ndn-tools?label=Latest%20version)
![Language](https://img.shields.io/badge/C%2B%2B-17-blue)
[![CI](https://github.com/named-data/ndn-tools/actions/workflows/ci.yml/badge.svg)](https://github.com/named-data/ndn-tools/actions/workflows/ci.yml)
[![Docker](https://github.com/named-data/ndn-tools/actions/workflows/docker.yml/badge.svg)](https://github.com/named-data/ndn-tools/actions/workflows/docker.yml)
[![Docs](https://github.com/named-data/ndn-tools/actions/workflows/docs.yml/badge.svg)](https://github.com/named-data/ndn-tools/actions/workflows/docs.yml)

This repository contains a collection of basic tools for [Named Data Networking (NDN)](https://named-data.net/).
The tools currently included are:

* [**ndnpeek** and **ndnpoke**](tools/peek): transmit a single Interest/Data packet
  between a consumer and a producer
* [**ndnget**](tools/get) and [**ndnserve**](tools/serve): segmented file transfer
  between a consumer and a producer
* [**ndnping**](tools/ping): test reachability between two NDN nodes
* [**ndndump**](tools/dump): capture and analyze live traffic on an NDN network
* [**ndndissect**](tools/dissect): inspect the TLV structure of an NDN packet
* [**dissect-wireshark**](tools/dissect-wireshark): Wireshark extension to inspect
  the TLV structure of NDN packets

## Installation

See [`INSTALL.md`](INSTALL.md) for build instructions.

## Reporting bugs

Please submit any bug reports or feature requests to the
[ndn-tools issue tracker](https://redmine.named-data.net/projects/ndn-tools/issues).

## Contributing

Contributions to ndn-tools are greatly appreciated and can be made through our
[Gerrit code review site](https://gerrit.named-data.net/).
If you are new to the NDN software community, please read our
[Contributor's Guide](https://github.com/named-data/.github/blob/main/CONTRIBUTING.md)
and [`README-dev.md`](README-dev.md) to get started.

## License

ndn-tools is free software distributed under the GNU General Public License version 3.
See [`COPYING.md`](COPYING.md) for details.
