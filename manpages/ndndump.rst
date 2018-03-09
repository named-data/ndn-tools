ndndump
=======

Usage
-----

::

    ndndump [-hV] [-i INTERFACE] [-r FILE] [-f FILTER] [EXPRESSION]

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

.. option:: -h

    Print help and exit.

.. option:: -V

    Print version and exit.

.. option:: -i INTERFACE

    Listen on the specified interface.
    If unspecified, ndndump searches the system interface list for the lowest numbered,
    configured up interface (excluding loopback).

.. option:: -r FILE

    Read packets from the specified file (which was created with :manpage:`tcpdump(8)` using its ``-w`` option).

.. option:: -v

    Produce verbose output.

.. option:: -f FILTER

    Print a packet only if its Name matches the regular expression ``FILTER``.

.. option:: EXPRESSION

    Selects which packets will be analyzed, in :manpage:`pcap-filter(7)` format.
    If no :option:`EXPRESSION` is given, a default expression is implied which can be seen with ``-h`` option.

Examples
--------

Capture on eth1 and print packets containing "ping":

::

    ndndump -i eth1 -f '.*ping.*'
