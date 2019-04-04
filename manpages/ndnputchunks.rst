ndnputchunks
============

Synopsis
--------

::

    ndnputchunks [options] ndn:/name

Description
-----------

:program:`ndnputchunks` is a producer program that reads a file from the standard input, and makes it available as NDN Data segments.

It appends version and segment number components to the specified name, according to the `NDN naming conventions`_.

.. _NDN naming conventions: http://named-data.net/publications/techreports/ndn-tr-22-ndn-memo-naming-conventions/

Options
-------

.. option:: -h, --help

    Print help and exit.

.. option:: -f, --freshness MILLISECS

    FreshnessPeriod of the published Data packets, in milliseconds. Default = 10000 [ms].

.. option:: -p, --print-data-version

    Print version of the published Data to the standard output.

.. option:: -s, --size BYTES

    Maximum chunk size, in bytes. Default = 4400 [bytes].

.. option:: -S, --signing-info STRING

    Signing information. Can be set to "id:/localhost/identity/digest-sha256" in order to speed up signing.
    However, keep in mind that this only a hash function and not a real signature.
    Other options are found in the `ndn-cxx documentation for SigningInfo`_.

    .. _ndn-cxx documentation for SigningInfo: https://named-data.net/doc/ndn-cxx/0.6.5/doxygen/d8/dc8/classndn_1_1security_1_1SigningInfo.html#afc960f9f5da5536b958403dc7b701826

.. option:: -q, --quiet

    Turn off all non-error output.

.. option:: -v, --verbose

    Produce verbose output.

.. option:: -V, --version

    Print program version and exit.


Examples
--------

The following command will publish the text of the GPL-3 license under the `/localhost/demo/gpl3`
prefix::

    ndnputchunks /localhost/demo/gpl3 < /usr/share/common-licenses/GPL-3

To find the published version you have to start ndnputchunks with the `-p` command line option,
for example::

    ndnputchunks -p /localhost/demo/gpl3 < /usr/share/common-licenses/GPL-3

This command will print the published version to the standard output.

To publish Data with a specific version, you must append a version component to the end of the
prefix. The version component must follow the aforementioned NDN naming conventions.
For example, the following command will publish the version `%FD%00%00%01Qc%CF%17v` of the
`/localhost/demo/gpl3` prefix::

    ndnputchunks /localhost/demo/gpl3/%FD%00%00%01Qc%CF%17v < /usr/share/common-licenses/GPL-3

If the version component is not valid, a new well-formed version will be generated and appended
to the supplied NDN name.


Notes
-----

.. target-notes::
