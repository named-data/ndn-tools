# ndn-dissect

**ndn-dissect** is an NDN packet format inspector.
It reads zero or more NDN packets from either an input file or the standard input,
and displays the Type-Length-Value (TLV) structure of those packets on the standard output.

Usage example:

1. start NFD on local machine
2. execute `echo 'HELLO WORLD' | ndnpoke ndn:/localhost/demo/hello`
3. on another console, execute `ndnpeek ndn:/localhost/demo/hello | ndn-dissect`

For more information, consult the manpage.
