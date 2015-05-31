ndnping
=======

Usage
-----

::

    ndnping [-h] [-V] [-i interval] [-o timeout] [-c count] [-n start] [-p identifier] [-a] [-t] prefix

Description
-----------

``ndnping`` is a reachability testing tool that sends Interest packets and expects Data packets in
response. If Data packets are not received within the timeout period for their Interest,
``ndnping`` marks them as timed out. If ``ndnping`` does receive a Data packet within the timeout,
the time from sending the Interest to receiving the Data is recorded and displayed. Once
``ndnping`` either reaches the specified total number of Interests to be sent or receives an
interrupt signal, it prints statistics on the Interests, including the number of Interests sent,
the number of Data packets received, and the average response time of the Data.

``prefix`` is interpreted as the Interest prefix. The name of the sent Interest consists of the
prefix followed by "ping", the optional identifier specified by the '-p' option, and finally a
sequence number as a decimal number string.

Options
-------

``-h``
  Print help and exit.

``-V``
  Print version and exit.

``-i``
  Set ping interval in milliseconds (default 1000ms - minimum 1ms).

``-o``
  Set ping timeout in milliseconds (default 4000ms).

``-c``
  Set total number of Interests to be sent.

``-n``
  Set the starting sequence number, which is incremented by 1 after each Interest sent.

``-p``
  Adds specified identifier to the Interest names before the numbers to avoid conflict.

``-a``
  Allows routers to return stale Data from cache.

``-t``
  Prints a timestamp with received Data and timeouts.

Examples
--------

Test reachability for ``ndn:/edu/arizona`` using 4 Interest packets and print timestamp

::

    ndnping -c 4 -t ndn:/edu/arizona