ndnping
=======

Usage
-----

::

    ndnpingserver [-h] [-V] [-x freshness] [-p satisfy] [-t] [-s size] prefix

Description
-----------

``ndnpingserver`` listens for the specified Interest prefix and sends Data packets when an Interest
under that prefix is received. Once ``ndnpingserver`` either reaches the specified total number of
Interests to be satisfied or receives an interrupt signal, it prints the number of Data packets
sent.

``prefix`` is interpreted as the Interest prefix to listen for. The FreshnessPeriod of Data packets
is set with the -x option (default 1000ms). The content is by default empty, but if a size is
specified with the '-s' option, it contains the specified number of the letter "a". Finally, the
Data is signed with an SHA256 digest.

Options
-------

``-h``
  Print help and exit.

``-V``
  Print version and exit.

``-x``
  Set freshness period in milliseconds (minimum 1000ms).

``-p``
  Set maximum number of pings to satisfy.

``-t``
  Prints a timestamp with received Data and timeouts.

``-s``
  Specify the size of the response payload.

Examples
--------

Listen on ``ndn:/edu/arizona`` and response to at most 4 pings, printing timestamp on each received
ping

::

    ndnpingserver -p 4 -t ndn:/edu/arizona