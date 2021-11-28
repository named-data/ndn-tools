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
- In packet 1, the "Info" column of Wireshark shows: `Interest /ndn/broadcast/ndnrtc-chatrooms/2366d310b15ba97a62c8734d4a760174f78d14446be3d70a54ee80d3ed19c83b`
- In packet 7, the "Info" column of Wireshark shows: `Data /example/testApp/1/testApp/%FD%00%00%01O%23E%07%ED`

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

### 9. NDNLPv2

Trace file: `ndnlpv2.pcap`

Trace summary: Handcrafted NDNLPv2 packets.
(`xxd -p -r < ndnlpv2.hex > ndnlpv2.pcap`).

Expected result of the dissection: 12 "Ethernet (NDN)" frames.
1.  LpPacket contains 12 unrecognized fields. The 10th and 12th fields are labelled "ignored".
2.  No special requirements.
3.  "Sequence: 12732154106453800448". "FragIndex: 0". "FragCount: 2". Fragment exists.
4.  "Sequence: 12732154106453800449". "FragIndex: 1". "FragCount: 2". Fragment exists.
5.  "NackReason: Congestion". The "Info" column shows "Nack /A".
6.  "NackReason: Duplicate".
7.  "NackReason: NoRoute".
8.  "NackReason: 1".
9.  Nack exists, but NackReason does not exist.
10. "PitToken: 15047039637272254167".
11. "CongestionMark: 1". "TxSequence: 16204482402681480935".
12. There are eight Ack fields. First is "Ack: 16204482402681480704". Eighth is "Ack: 16204482402681480711".

### 10. NDNLPv2 (random)

Trace file: `ndnlpv2-more.pcap`

Trace summary: Short capture of randomly generated NDNLPv2 packets (see
https://gist.github.com/cawka/fcdde58cc4dc94d789025ab8300076dc) with multiple fields set to various
values. Given the random generation, some fields are semantically meaningless.

Expected results of the dissection:
- 10 NDN (NDNLPv2) packets extracted from the reassembled TCP stream
- the dissection results include Fragment (Interest/Data), Sequence (number), FragIndex (number),
  FragCount (number), Nack (various reasons), NextHopFaceId, IncomingFaceId, CachingPolicy, and
  unknown fields

### 11. NDN Packet Format v0.3

Trace file: `packet03.pcap`

Trace summary: Handcrafted packets in NDN Packet Format v0.3
(`xxd -p -r < packet03.hex > packet03.pcap`).
All packets are valid and do not contain unrecognized TLV elements.

Expected results of the dissection:
- Packet 1 is recognized as "Interest" and contains `CanBePrefix: Yes`, `MustBeFresh: Yes`,
  `HopLimit: 214`, as well as a "ForwardingHint" and an "ApplicationParameters" field.
- Packet 2 is recognized as "Interest" and has `Name: /params-sha256=41/7=B/C/252=D/256=E/65535=E/sha256digest=ee357c5791dcaa4494d9b301047b875d8833caa76dada3e95837bbc3eaf7b300`.
- Packet 3 is recognized as "Data" and has `Name: /`.

### 12. URI Scheme

Trace file: `nameuri.pcap`

Trace summary: Handcrafted packet for testing URI encoding in Name and FinalBlockId
(`xxd -p -r < nameuri.hex > nameuri.pcap`).

Expected results of the dissection:
- Packet 1 is recognized as "Data".
- Its name has eight components.
- First name component is `NameComponent: ...`.
- Second name component is `NameComponent: ....`.
- Third name component is `NameComponent: .....`.
- Fourth name component is `NameComponent: .A`.
- Fifth name component is `NameComponent: %00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F%20%21%22%23%24%25%26%27%28%29%2A%2B%2C-.%2F0123456789%3A%3B%3C%3D%3E%3F`.
  Notice that digits, HYPHEN (`-`), and PERIOD (`.`) are not percent-encoded.
- Sixth name component is `NameComponent: %40ABCDEFGHIJKLMNOPQRSTUVWXYZ%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz%7B%7C%7D~%7F`.
  Notice that upper case letters, lower case letters, UNDERSCORE (`_`), and TILDE (`~`) are not percent-encoded.
- Seventh name component is `NameComponent: %80%81%82%83%84%85%86%87%88%89%8A%8B%8C%8D%8E%8F%90%91%92%93%94%95%96%97%98%99%9A%9B%9C%9D%9E%9F%A0%A1%A2%A3%A4%A5%A6%A7%A8%A9%AA%AB%AC%AD%AE%AF%B0%B1%B2%B3%B4%B5%B6%B7%B8%B9%BA%BB%BC%BD%BE%BF`.
- Eighth name component is `NameComponent: %C0%C1%C2%C3%C4%C5%C6%C7%C8%C9%CA%CB%CC%CD%CE%CF%D0%D1%D2%D3%D4%D5%D6%D7%D8%D9%DA%DB%DC%DD%DE%DF%E0%E1%E2%E3%E4%E5%E6%E7%E8%E9%EA%EB%EC%ED%EE%EF%F0%F1%F2%F3%F4%F5%F6%F7%F8%F9%FA%FB%FC%FD%FE%FF`.
- FinalBlockId and its nested NameComponent are both `%02`.

### 13. PPP

Trace file: `ppp.pcap`

Trace summary: 8 packets created in a simple ndnSIM scenario

Expected results of the dissection:
- 4 interests (packets 1, 2, 5, 6), each carried in an LpPacket fragment
- 4 data packets (packets 3, 4, 7, 8), also each carried in an LpPacket fragment
