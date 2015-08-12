NDN Packet Dissector for Wireshark
==================================

**NDN packet dissector requires at least version 1.12.6 of Wireshark with LUA support enabled**

The dissection of [Named Data Networking (NDN) packets](http://named-data.net/doc/ndn-tlv/) is
supported in the following cases:

- NDN packets are encapsulated in IPv4/IPv6 UDP packets with source or destination port
  6363 or 56363.

- NDN packets are encapsulated in IPv4/IPv6 TCP segments with source or destination
  port 6363.

- NDN packets are encapsulated in IPv4/IPv6 TCP/HTTP WebSocket packets with source or
  destination port 9696.

- NDN packets are encapsulated in Ethernet frames with EtherType 0x8624.

## Available dissection features

- When UDP packet is fragmented, the dissection is performed after the full IP reassembly.
  If the full reassembly is not possible (e.g., a wrong checksum or missing segments),
  dissection is not performed.

- When multiple NDN packets are part of a single UDP datagram, TCP segment, or WebSocket
  payload, all NDN packets are dissected.

- When a single NDN packet is scattered across multiple TCP segments or WebSocket
  payloads, it is dissected after the successful reconstruction of the necessary portion
  of the TCP stream.  If the reconstruction of the necessary portion of the TCP stream is
  not possible (e.g., missing segments), the dissection is not performed.

- When an NDN packet is not aligned to the segment or payload boundary, the dissector
  searches for any valid NDN packet within the segment using heuristics defined by the
  following pseudocode:

        for each offset in range (0, packet length)
          type <- read TLV VarNumber from (buffer + offset)
          length <- read TLV VarNumber from (buffer + offset + length of type field)

          if type is either 5 or 6  // Type of NDN Interest of Data packet)
             and length is less 8800 // Current (soft) limit for NDN packet size
          then
             dissect NDN packet from (buffer + offset)
          end if

Currently, the dissector does not support NDNLPv2 packets.

## Usage

By default, the dissector script `ndn.lua` is installed into `/usr/local/share/ndn-dissect-wireshark`.
On some platforms, it may also be installed in `/usr/share/ndn-dissect-wireshark` or
`/opt/local/share/ndn-dissect-wireshark`.  To enable the dissector for Wireshark session,
use `-X` command line option, specifying the full path to the `ndn.lua` script:

    wireshark -X lua_script:/usr/local/share/ndn-dissect-wireshark/ndn.lua

Similarly, NDN packets dissector can be enabled when using `tshark`:

    tshark shark -X lua_script:/usr/local/share/ndn-dissect-wireshark/ndn.lua

To enable NDN packets dissector for all future Wireshark sessions, you can create/edit
Wireshark's `init.lua` script, which located in `/usr/share/wireshark`,
`/usr/local/share/wireshark`, `/Applications/Wireshark.app/Contents/Resources/share/wireshark`,
or similar location depending on the platform and the way Wireshark is installed.  The
`dofile` command should be added to the end of `init.lua` file:

    -- dofile("/full/path/to/ndn.lua")
    dofile("/usr/local/share/ndn-dissect-wireshark/ndn.lua")

For more detailed information about how to use Lua refer to [Lua wiki](https://wiki.wireshark.org/Lua).

## Known issues

Due to security issues, customized lua scripts are not allowed to be loaded when Wireshark
is started with root privileges.  There are two workarounds:

- run Wireshark, `dumpcap`, or `tcpdump` with root privileges to capture traffic to a file, later
  running Wireshark without root privileges and to analyze the captured traffic.

- (beware of potential security implications) allow non-root users to capture packets:

  * On Linux platform, you can use `setcap`

          sudo setcap cap_net_raw,cap_net_admin=eip /full/path/to/wireshark

      You may need to install a package to use setcap (e.g., `sudo apt-get install libcap2-bin` on Ubuntu)

  * On Debian/Ubuntu Linux, capturing traffic with Wireshark by a non-root user can be enabled by adding
    this user to the `wireshark` group.

    See [Wireshark Debian README](http://anonscm.debian.org/viewvc/collab-maint/ext-maint/wireshark/trunk/debian/README.Debian?view=markup)
    for more details.

  * On OSX platform, `/dev/bpf*` devices need to be assigned proper permissions

    Automatically using ChmodBPF app

        curl https://bugs.wireshark.org/bugzilla/attachment.cgi?id=3373 -o ChmodBPF.tar.gz
        tar zxvf ChmodBPF.tar.gz
        open ChmodBPF/Install\ ChmodBPF.app

    or manually:

        sudo chgrp admin /dev/bpf*
        sudo chmod g+rw /dev/bpf*
