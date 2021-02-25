ndn-dissect
===========

Synopsis
--------

**ndn-dissect** [**-h**] [**-c**] [**-V**] [*file*]

Description
-----------

:program:`ndn-dissect` is an NDN packet format inspector.
It reads zero or more NDN packets from either an input file or the standard
input, and displays the Type-Length-Value (TLV) structure of those packets
on the standard output.

Options
-------

.. option:: -h, --help

    Print help and exit.

.. option:: -c, --content

    Dissect the value of Content elements as well. By default, the value of a
    Content element is treated as an opaque blob and is not dissected further.

.. option:: -V, --version

    Print program version and exit.

.. option:: file

    The file to read packets from.
    If no *file* is given, or if *file* is "-", the standard input is used.

Examples
--------

Inspect the response to Interest ``/app1/video``::

    ndnpeek /app1/video | ndn-dissect
