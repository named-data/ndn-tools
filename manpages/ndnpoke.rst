ndnpoke
=======

Synopsis
--------

**ndnpoke** [-h] [-f] [-F] [-x *freshness*] [-i *identity*\|\ -D] [-w *timeout*] [-V] *name*

Description
-----------

:program:`ndnpoke` is a simple producer program that reads a payload from the standard
input and publishes it as a single Data packet. The Data packet is either sent as a
response to an incoming Interest matching *name*, or immediately pushed to the local
NDN forwarder as "unsolicited Data" if the **-f** flag is specified.

Options
-------

``-h, --help``
  Print help and exit.

``-f, --force``
  Send the Data packet without waiting for an incoming Interest.

``-F, --final``
  Set the ``FinalBlockId`` to the last component of *name*.

``-x, --freshness <freshness>``
  Set ``freshness`` (in milliseconds) as the ``FreshnessPeriod``.

``-i, --identity <identity>``
  Use ``identity`` to sign the Data packet.

``-D, --digest``
  Use ``DigestSha256`` signature type instead of the default ``SignatureSha256WithRsa``.

``-w, --timeout <timeout>``
  Quit the program after ``timeout`` milliseconds, even if no Interest has been received.

``-V, --version``
  Print version and exit.

Exit Status
-----------

0: Success

1: An unspecified error occurred

2: Malformed command line

3: No Interests received before the timeout

5: Prefix registration failed

Example
-------

Create a Data packet with content ``hello`` and name ``/app/video`` and wait at
most 3 seconds for a matching Interest to arrive::

    echo "hello" | ndnpoke -w 3000 /app/video
