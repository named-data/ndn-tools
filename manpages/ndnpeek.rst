ndnpeek
=======

Usage
-----

::

    ndnpeek [-h] [-f] [-r] [-m min] [-M max] [-l lifetime] [-p] [-w timeout] [-v] [-V] name

Description
-----------

``ndnpeek`` is a simple consumer program that sends one Interest and expects one Data
packet in response.  The full Data packet (in TLV format) is written to stdout.  The
program terminates with return code 0 if Data arrives, and with return code 1 when timeout
occurs.

``name`` is interpreted as the Interest name.

Options
-------

``-h, --help``
  Print help and exit.

``-f, --fresh``
  If specified, set ``MustBeFresh`` selector in the Interest packet.

``-r, --rightmost``
  Set ``ChildSelector=1`` (the rightmost child) selector.

``-m, --minsuffix min``
  Set ``min`` as the ``MinSuffixComponents`` selector.

``-M, --maxsuffix max``
  Set ``max`` as the ``MaxSuffixComponents`` selector.

``-l, --lifetime lifetime``
  Set ``lifetime`` (in milliseconds) as the ``InterestLifetime``.

``-p, --payload``
  If specified, print the received payload only, not the full packet.

``-w, --timeout timeout``
  Timeout after ``timeout`` milliseconds.

``-v, --verbose``
  If specified, verbose output.

``-V, --version``
  Print version and exit.

``--link-file [file]``
  Read Link object from ``file`` and add it to the expressed Interest.

Exit Codes
----------

0: Success

1: An unspecified error occurred

2: Malformed command line

3: Network operation timed out

4: Nack received

Examples
--------

Send Interest for ``ndn:/app1/video`` and print the received payload only

::

    ndnpeek -p ndn:/app1/video
