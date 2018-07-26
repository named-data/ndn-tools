ndndump
=======

Usage
-----

::

    ndndump [-hvV] [-i INTERFACE] [-r FILE] [-f FILTER] [PCAP-FILTER]

Description
-----------

:program:`ndndump` is a traffic analysis tool that captures NDN packets on the wire and
displays brief information about them.

Currently, :program:`ndndump` is capable of extracting Interest, Data, and Nack packets from:

* Ethernet frame
* PPP link (e.g., pcap trace from ndnSIM)
* IPv4 UDP unicast tunnel
* IPv4 UDP multicast group
* IPv4 TCP tunnel, when Interest/Data/Nack is aligned to the beginning of a TCP segment

For more complex scenarios, including the case of NDN packets that span multiple IP fragments
or multiple TCP segments, it is recommended to use the **NDN Wireshark dissector**, either via
:manpage:`wireshark(1)` or :manpage:`tshark(1)`.

Options
-------

.. option:: -h, --help

    Print help and exit.

.. option:: -i INTERFACE, --interface=INTERFACE

    Listen on *INTERFACE*.
    If unspecified, ndndump searches the system interface list for the lowest numbered,
    configured up interface (excluding loopback).
    On Linux, a value of "any" can be used to capture packets from all interfaces.
    Note that captures on the "any" pseudo-interface will not be done in promiscuous mode.

.. option:: -r FILE, --read=FILE

    Read packets from *FILE*, which can be created by :manpage:`tcpdump(8)` with its
    ``-w`` option, or by similar programs.

.. option:: -f FILTER, --filter=FILTER

    Print a packet only if its name matches the regular expression *FILTER*.

.. option:: -p, --no-promiscuous-mode

    Do not put the interface into promiscuous mode.

.. option:: -t, --no-timestamp

    Do not print a timestamp for each packet.

.. option:: -v, --verbose

    Produce verbose output.

.. option:: -V, --version

    Print ndndump and libpcap version strings and exit.

.. option:: PCAP-FILTER

    :option:`PCAP-FILTER` is an expression in :manpage:`pcap-filter(7)` format that
    selects which packets will be analyzed.
    If no :option:`PCAP-FILTER` is given, a default filter is used. The default filter
    can be seen with the :option:`--help` option.

Examples
--------

Capture on eth1 and print packets containing "ping"::

    ndndump -i eth1 -f '.*ping.*'
