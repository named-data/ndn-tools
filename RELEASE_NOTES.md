Release Notes
=============

## Version 0.5

all:
- Switch to version 2 of certificates, `KeyChain`, and `Validator` (Issue #4089)
- Compilation fixes (Issue #4259)

peek:
- Convert use of `Link` into `ForwardingHint` (Issue #4055)

chunks:
- Make `ndnputchunks` display some output by default. A new `-q` flag makes output
  completely silent, except for errors (Issue #4286)
- Refactor `ndnputchunks` options handling
- Reduce initial timeout of iterative version discovery in `ndncatchunks` (Issue #4291)
- Fix potential `ndncatchunks` crash on exit

## Version 0.4

**NOTE**

  As of version 0.4, NDN Essential Tools require a modern compiler (gcc >= 4.8.2, clang >= 3.4)
  and a relatively new version of the Boost libraries (>= 1.54).  This means that the code no
  longer compiles with the packaged version of gcc and Boost libraries on Ubuntu 12.04.
  NDN Essential Tools can still be compiled on such systems, but require a separate
  installation of a newer version of the compiler (e.g., clang-3.4) and dependencies.

chunks:
- Change default version discovery to `iterative`
- Improve help text of `ndnputchunks`
- Fix `DiscoverVersionIterative` build error
- Modularize Interest pipeline implementation
- Add AIMD congestion control (Issue #3636)
- Code cleanup and improvements

dissect-wireshark:
- Add initial support for NDNLPv2 (Issue #3197)
- Fix potential memory overflow

dump:
- Add support for Linux cooked-mode capture (SLL) (Issue #3061)
- Improve error messages

pib:
- Disable by default (can be compiled with ndn-cxx version 0.5.0)
- Fix compilation error with new version of ndn-cxx library
- Avoid use of deprecated block helpers
- Correct build target path

ping:
- Recognize and trace NACK
- Fix potential divide-by-zero bug in StatisticsCollector (Issue #3504)

peek:
- Recognize and properly handle NACK
- Refactor implementation

## Version 0.3

chunks: **New** (pair of) tool(s) for segmented file transfer

peek:
- Allow verbose output
- Switch from `getopt` to `boost::program_options`
- Add `--link-file` option

ping:
- Document ndnping protocol

dump:
- Capture and print network NACK packets
- Update docs to include NACK capture feature

build scripts:
- Enable -Wextra by default
- Fix missing tool name in `configure --help` output
- Fix compatibility with Python 3

## Version 0.2

Code improvements and new tools:

- PIB service to manage the public information of keys and publish certificates
  (Issue 3018)
- A Wireshark dissector for NDN packets (Issue 3092)

## Version 0.1

Initial release of NDN Essential Tools, featuring:

- ndnpeek, ndnpoke: a pair of programs to request and make available for
  retrieval of a single Data packet
- ndnping, ndnpingserver: reachability testing tools for Named Data Networking
- ndndump: a traffic analysis tool that captures Interest and Data packets on
  the wire
- ndn-dissect: an NDN packet format inspector. It reads zero or more NDN
  packets from either an input file or the standard input, and displays the
  Type-Length-Value (TLV) structure of those packets on the standard output.
