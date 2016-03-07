ndndump
=======

Usage
-----

::

    ndndump [-hV] [-i interface] [-r file] [-f filter] [expression]

Description
-----------

:program:`ndndump` is a traffic analysis tool that captures Interest, Data, and Nack packets on the
wire and displays brief information about captured packets.

Currently, :program:`ndndump` is capable of extracting Interest, Data, and Nack packets from:

* Ethernet
* PPP link (e.g., pcap trace from ndnSIM)
* IPv4 UDP unicast tunnel
* IPv4 UDP multicast group
* IPv4 TCP tunnel, when Interest/Data/Nack is aligned to the front of a TCP segment

Options
-------

``-h``
  Print help and exit.

``-V``
  Print version and exit.

``-i``
  Listen on :option:`interface`.
  If unspecified, ndndump searches the system interface list for the lowest numbered,
  configured up interface (excluding loopback).

``-r``
  Read packets from :option:`file` (which was created with :manpage:`tcpdump(8)` using its -w option).

``-v``
  Produce verbose output.

``-f``
  Print a packet only if its Name matches the regular expression :option:`filter`.

``expression``
  Selects which packets will be analyzed, in :manpage:`pcap-filter(7)` format.
  If no :option:`expression` is given, a default expression is implied which can be seen with ``-h`` option.

Examples
--------

Capture on eth1 and print packets containing "ping":

::

    ndndump -i eth1 -f '.*ping.*'
