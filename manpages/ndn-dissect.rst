ndn-dissect
===========

Usage
-----

::

    ndn-dissect [-hV] [INPUT-FILE]

Description
-----------

``ndn-dissect`` is an NDN packet format inspector.
It reads zero or more NDN packets from either an input file or the standard input,
and displays the Type-Length-Value (TLV) structure of those packets on the standard output.

Options
-------

.. option:: -h

  Print help and exit.

.. option:: -V

  Print version and exit.

.. option:: INPUT-FILE

  The file to read packets from.
  If no :option:`INPUT-FILE` is given, the standard input is used.

Examples
--------

Inspect the response to Interest ``ndn:/app1/video``

::

    ndnpeek ndn:/app1/video | ndn-dissect
