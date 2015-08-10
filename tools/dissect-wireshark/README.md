ndn-dissect-wireshark
=====================

A Wireshark dissector for [Named Data Networking (NDN) packets](http://named-data.net/doc/ndn-tlv/).

The dissector is able to process and visualize structure of NDN packets encapsulated in
IPv4/IPv6 UDP packets with source of destination port 6363, IPv4/IPv6 TCP packets with
source or destination port 6363, IPv4/IPv6 TCP/HTTP WebSocket packets (any port).

Note that when UDP packet is fragmented, only the first fragment is getting dissected.
For TCP packets, the dissector assumes that NDN packet starts at the packet boundary,
therefore some NDN packets will not be properly dissected.  The same limitation applies to
WebSocket packets.

Currently, the dissector does not support NDNLPv2 packets, Link, SelectedDelegation fields.

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
