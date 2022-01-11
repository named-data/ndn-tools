ndnpeek
=======

Synopsis
--------

**ndnpeek** [-h] [-P] [-f] [-l *lifetime*] [-H *hops*] [-A *parameters*]
[-p] [-w *timeout*] [-v] [-V] *name*

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

``-F, --fwhint <name>``
  Add a delegation name to the ``ForwardingHint`` of the Interest packet.
  This option may be repeated to specify multiple delegation names.

``-l, --lifetime <lifetime>``
  Set ``lifetime`` (in milliseconds) as the ``InterestLifetime``.

``-H, --hop-limit <hops>``
  Set the Interest's ``HopLimit`` to the specified number of hops.

``-A, --app-params <parameters>``
  Set the Interest's ``ApplicationParameters`` from a base64-encoded string.

``--app-params-file <file>``
  Set the Interest's ``ApplicationParameters`` from the specified file.

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

Examples
--------

Send an Interest for ``/app1/video`` and print the received payload only::

    ndnpeek -p /app1/video

Send an Interest for ``/app2/foo``, requesting fresh content, with an InterestLifetime
of 8 seconds, and with the ApplicationParameters containing the ASCII string "hello";
print the performed operations verbosely but discard the received Data packet::

    ndnpeek -vf -l 8000 -A "aGVsbG8=" /app2/foo >/dev/null
