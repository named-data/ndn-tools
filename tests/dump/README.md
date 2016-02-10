Testing Instructions
====================

This folder contains a tcpdump trace that can be used to manually check
the correctness of ndndump.

## Test Cases / Trace File Description

### 1. NDNLPv2 NACK

Trace file: `nack.pcap`

Trace summary: Six IPv4 UDP packets, carrying NDN interests and NACK packets.
Twelve IPv4 TCP packets, carrying NDN interests and NACK packets.

Expected result of the capture:

- 3 NDN interests are captured with UDP tunnel type.
- 3 NACKs are captured with UDP tunnel type and the NACK reasons:
  - Congestion
  - Duplicate
  - None

- 3 NDN interests are captured with TCP tunnel type.
- 4 NACKs are captured with TCP tunnel type and the NACK reasons:
  - Congestion
  - Duplicate
  - None
  - None
