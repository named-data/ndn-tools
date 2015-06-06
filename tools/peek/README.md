# ndnpeek and ndnpoke

**ndnpeek** and **ndnpoke** are a pair of programs to request and make available for retrieval of a single Data packet.

* **ndnpeek** is a consumer program that sends one Interest and expects one Data.
* **ndnpoke** is a producer program that serves one Data in response to an Interest.

Usage example:

1. start NFD on local machine
2. execute `echo 'HELLO WORLD' | ndnpoke ndn:/localhost/demo/hello`
3. on another console, execute `ndnpeek -p ndn:/localhost/demo/hello`

For more information, consult manpages of these programs.
