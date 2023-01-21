# ndndump

**ndndump** is a traffic analysis tool that captures Interest, Data, and Nack packets on the wire
and displays brief information about captured packets.

Usage example:

1. start NFD on the local machine
2. create a UDP tunnel to a remote machine
3. run `sudo ndndump`
4. cause some NDN traffic to be sent/received on the tunnel
5. observe the output of `ndndump`

For more information, consult the manpage.
