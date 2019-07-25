ndnpeek
=======

Synopsis
--------

**ndnpeek** [-h] [-P] [-f] [-l *lifetime*] [-p] [-w *timeout*] [-v] [-V] *name*

Description
-----------

:program:`ndnpeek` is a simple consumer program that sends one Interest and
expects one Data packet in response. By default, the full Data packet (in TLV
format) is written to the standard output.

*name* is interpreted as the Interest name.

Options
-------

``-h, --help``
  Print help and exit.

``-P, --prefix``
  If specified, include ``CanBePrefix`` element in the Interest packet.

``-f, --fresh``
  If specified, include ``MustBeFresh`` element in the Interest packet.

``--link-file <file>``
  Read Link object from ``file`` and add it as ``ForwardingHint`` to the Interest packet.

``-l, --lifetime <lifetime>``
  Set ``lifetime`` (in milliseconds) as the ``InterestLifetime``.

``-p, --payload``
  If specified, print the received payload only, not the full packet.

``-w, --timeout <timeout>``
  Quit the program after ``timeout`` milliseconds, even if no reply has been received.

``-v, --verbose``
  Turn on verbose output.

``-V, --version``
  Print version and exit.

Exit Status
-----------

0: Success

1: An unspecified error occurred

2: Malformed command line

3: Operation timed out

4: Nack received

Example
-------

Send Interest for ``/app1/video`` and print the received payload only::

    ndnpeek -p /app1/video
