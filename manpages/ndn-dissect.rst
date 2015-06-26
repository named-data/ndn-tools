ndn-dissect
===========

Usage
-----

::

    ndn-dissect [-hV] [input-file]

Description
-----------

``ndn-dissect`` is an NDN packet format inspector.
It reads zero or more NDN packets from either an input file or the standard input,
and displays the Type-Length-Value (TLV) structure of those packets on the standard output.

Options
-------

``-h``
  Print help and exit.

``-V``
  Print version and exit.

``input-file``
  The file to read packets from.
  If no :option:`input-file` is given, the standard input is used.

Examples
--------

Inspect the response to Interest ``ndn:/app1/video``

::

    ndnpeek ndn:/app1/video | ndn-dissect
