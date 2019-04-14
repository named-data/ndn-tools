# NDN Essential Tools

![Language](https://img.shields.io/badge/C%2B%2B-14-blue.svg)
[![Build Status](https://travis-ci.org/named-data/ndn-tools.svg?branch=master)](https://travis-ci.org/named-data/ndn-tools)
![Latest Version](https://img.shields.io/github/tag/named-data/ndn-tools.svg?color=darkkhaki&label=latest%20version)

**ndn-tools** is a collection of basic tools for [Named Data Networking](https://named-data.net/).
Tools in this collection include:

* [peek](tools/peek): transmit a single Interest/Data packet between a consumer
  and a producer
* [chunks](tools/chunks): segmented file transfer between a consumer and a producer
* [ping](tools/ping): test reachability between two NDN nodes
* [dump](tools/dump): capture and analyze live traffic on an NDN network
* [dissect](tools/dissect): inspect the TLV structure of an NDN packet
* [dissect-wireshark](tools/dissect-wireshark): Wireshark extension to inspect
  the TLV structure of NDN packets

## Installation

See [`INSTALL.md`](INSTALL.md) for build instructions.

## Reporting bugs

Please submit any bug reports or feature requests to the
[ndn-tools issue tracker](https://redmine.named-data.net/projects/ndn-tools/issues).

## Contributing

You're encouraged to contribute to ndn-tools!  If you are new to the NDN
software community, please read [`README-dev.md`](README-dev.md) and the
[Contributor's Guide](https://github.com/named-data/NFD/blob/master/CONTRIBUTING.md)
to get started.

## License

ndn-tools is an open source project licensed under the GPL version 3.
See [`COPYING.md`](COPYING.md) for more information.
