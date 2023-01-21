# ndnpeek and ndnpoke

**ndnpeek** and **ndnpoke** are a pair of programs to respectively request and serve a single Data packet.

* **ndnpeek** is a consumer program that sends one Interest and expects one Data.
* **ndnpoke** is a producer program that serves one Data in response to an Interest.

Usage example:

1. start NFD
2. run `echo 'HELLO WORLD' | ndnpoke /localhost/demo/hello`
3. in another terminal, run `ndnpeek -p /localhost/demo/hello`

For more information on these programs, consult their respective manpages.
