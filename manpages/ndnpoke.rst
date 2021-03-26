ndnpoke
=======

Synopsis
--------

**ndnpoke** [-h] [-f *freshness*] [-F] [-S *info*] [-u\|\ -w *timeout*] [-v] [-V] *name*

Description
-----------

:program:`ndnpoke` is a simple producer program that reads a payload from the standard
input and publishes it as a single Data packet. The Data packet is either sent as a
response to an incoming Interest matching *name*, or immediately pushed to the local
NDN forwarder as "unsolicited Data" if the **-u** flag is specified.

Options
-------

``-h, --help``
  Print help and exit.

``-f, --freshness <freshness>``
  Set ``freshness`` (in milliseconds) as the ``FreshnessPeriod``.

``-F, --final``
  Set the ``FinalBlockId`` to the last component of *name*.

``-S, --signing-info <info>``
  Specify the parameters used to sign the Data packet. If omitted, the default key of
  the default identity is used. The general syntax is ``<scheme>:<name>``. The most
  common supported combinations are as follows:

  * Sign with the default certificate of the default key of an identity: ``id:/<my-identity>``.
  * Sign with the default certificate of a specific key: ``key:/<my-identity>/ksk-1``.
  * Sign with a specific certificate: ``cert:/<my-identity>/KEY/ksk-1/ID-CERT/v=1``.
  * Sign with a SHA-256 digest: ``id:/localhost/identity/digest-sha256``. Note that this
    is only a hash function, not a real signature, but it can significantly speed up
    packet signing operations.

``-u, --unsolicited``
  Send the Data packet without waiting for an incoming Interest.

``-w, --timeout <timeout>``
  Quit the program after ``timeout`` milliseconds, even if no Interest has been received.

``-v, --verbose``
  Turn on verbose output.

``-V, --version``
  Print version and exit.

Exit Status
-----------

0: Success

1: An unspecified error occurred

2: Malformed command line

3: No Interests received before the timeout

5: Prefix registration failed

Examples
--------

Create a Data packet with content ``hello`` and name ``/app/video`` and wait at
most 3 seconds for a matching Interest to arrive::

    echo "hello" | ndnpoke -w 3000 /app/video
