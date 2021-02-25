ndnpingserver
=============

Synopsis
--------

**ndnpingserver** [-h] [-f *freshness*] [-p *count*] [-s *size*] [-t] [-q] [-V] *prefix*

Description
-----------

:program:`ndnpingserver` listens for the specified Interest prefix and sends Data packets when
an Interest under that prefix is received. Once :program:`ndnpingserver` either reaches the
specified total number of Interests to be satisfied or receives an interrupt signal, it prints
the number of Data packets sent.

*prefix* is interpreted as the Interest prefix to listen for. The FreshnessPeriod of Data packets
is set with the **-f** option (default 1 second). The content is by default empty, but if a size
is specified with the **-s** option, it contains the specified number of the letter "a". Finally,
the Data is signed with a SHA-256 digest.

Options
-------

``-h``
  Print help and exit.

``-f``
  Set freshness period in milliseconds.

``-p``
  Maximum number of pings to satisfy. A value of 0 means no limit.

``-s``
  Size of the response payload.

``-t``
  Print a timestamp before each log message.

``-q``
  Do not print a log message each time a ping is received.

``-V``
  Print version and exit.

Examples
--------

Listen on ``/edu/arizona`` and respond to at most 4 pings, printing the timestamp
on each received ping::

    ndnpingserver -p 4 -t /edu/arizona
