Testing Instructions
====================

This folder contains several crafted tcpdump traces that can be used to manually check
correctness of NDN Packet Dissector for Wireshark in several scenarios.

To use the dissector, follow the instructions in
[dissector's README.md](../../tools/dissect-wireshark/README.md).

`-r <trace-file>` command-line flag can be used to directly open a specific trace file in
WireShark.  For example:

    wireshark -X lua_script:../../tools/dissect-wireshark/ndn.lua -r ipv4-udp-fragmented.pcap

## Test Cases / Trace File Description

### 1. IPv4 UDP

Trace file: `ipv4-udp-fragmented.pcap`

Trace summary: several IPv4 UDP packets, carrying NDN interests and data packets. One
datagram is fragmented into several IPv4 packets.

Expected result of the dissection:

- NDN interests are dissected from packets 1, 2, 3, and 8.
- NDN data packet is dissected from defragmented packets 4, 5, 6, and 7.

### 2. IPv6 UDP

Trace file: `ipv6-udp-fragmented.pcap`

Trace summary: several IPv6 UDP packets, carrying NDN interests and data packets. One
datagram is fragmented into several IPv6 packets.

Expected result of the dissection:

- NDN interests are dissected from packets 1, and 2.
- NDN data packet is dissected from defragmented packets 3, 4, 5, and 6.

### 3. IPv4 TCP

#### 3.1. De-segmentation and dissection

  Trace file: `ipv4-tcp-segmented.pcap`

  Trace summary: several IPv4 TCP packets, carrying NDN interest and a data packets. The data
  packet spans several TCP segments.

  Expected result of the dissection:
  - interest packet is properly dissected from packet 2.
  - data packet is properly dissected after de-segmentation of payloads in packets 4, 5, 6, and 7.

#### 3.2. Dissection of TCP segments containing multiple NDN packets

  Trace file: `ipv4-tcp-multi-ndn-packets-in-segment.pcap`

  Trace summary: Several IPv4 TCP packets, payload of one containing several NDN interests.

  Expected result of the dissection:
  - a single interest packet is dissected from packet 1.
  - four interest packets are dissected from packet 3.

### 4. IPv6 TCP

Trace file: `ipv6-tcp-segmented.pcap`

Trace summary: several IPv6 TCP packets, carrying an NDN data packet that spans several
TCP segments.

Expected result of the dissection:
- data packet is properly dissected after de-segmentation of payloads in packets 2, 3, 4, and 5.

### 5. IPv4 TCP/WebSocket

Trace file: `ipv4-websocket-segmented.pcap`

Trace summary: Partial capture of a live IPv4 WebSocket session with a single NDN interest
retrieving large (~5k) NDN data packet.

Expected result of the dissection:
- interest packet is dissected after a partial reconstruction of WebSocket session at
  packet 16.
- data packet is properly dissected after a partial reconstruction of WebSocket
  conversation at packet 22.

### 6. IPv6 TCP/WebSocket

Trace file: `ipv6-websocket-segmented.pcap`

Trace summary: Partial capture of a live IPv6 WebSocket session with a single NDN interest
retrieving large (~5k) NDN data packet.

Expected result of the dissection:
- interest packet is dissected after a partial reconstruction of WebSocket session at
  packet 6.
- data packet is properly dissected after a partial reconstruction of WebSocket
  conversation at packet 12.

### 7. Ethernet

Trace file: `ethernet.pcap`

Trace summary: Short capture, containing an NDN interest multicasted directly in Ethernet frame.

Expected result of the dissection:
- interest packet is dissected from packet 6.

### 8. tvb Overflow

Trace file: `bug3603.pcap`

Trace summary: A Data whose payload could be read as incomplete TLV and may cause tvb overflow
if parser does not check packet length.

Expected result of the dissection:
- data packet is dissected at packet 12 without Lua error.
