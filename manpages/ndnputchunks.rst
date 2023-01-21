ndnputchunks
============

Synopsis
--------

**ndnputchunks** [*option*]... *name*

Description
-----------

:program:`ndnputchunks` is a producer program that reads a file from the standard input
and makes it available as NDN Data segments.

Version and segment number components are appended to the specified *name* as needed,
according to the `NDN naming conventions`_.

.. _NDN naming conventions: https://named-data.net/publications/techreports/ndn-tr-22-3-ndn-memo-naming-conventions/

Options
-------

.. option:: -h, --help

    Print help and exit.

.. option:: -f, --freshness MILLISECS

    FreshnessPeriod of the published Data packets, in milliseconds.
    The default is 10000 (10 seconds).

.. option:: -s, --size BYTES

    Maximum chunk size, in bytes. The default is 8000 bytes.

.. option:: -N, --naming-convention CONVENTION

    Select the convention used to encode NDN name components. The available choices are:

    * ``marker`` (shorthand: ``m`` or ``1``) for the old marker-based encoding;
    * ``typed`` (shorthand: ``t`` or ``2``) for the new encoding based on typed name components.

    If this option is not specified, the ndn-cxx library's default is used.

.. option:: -S, --signing-info STRING

    Specify the parameters used to sign the Data packets. If omitted, the default key
    of the default identity is used. The general syntax is ``<scheme>:<name>``. The
    most common supported combinations are as follows:

    * Sign with the default certificate of the default key of an identity: ``id:/<my-identity>``.
    * Sign with the default certificate of a specific key: ``key:/<my-identity>/ksk-1``.
    * Sign with a specific certificate: ``cert:/<my-identity>/KEY/ksk-1/ID-CERT/v=1``.
    * Sign with a SHA-256 digest: ``id:/localhost/identity/digest-sha256``. Note that this
      is only a hash function, not a real signature, but it can significantly speed up
      packet signing operations.

.. option:: -p, --print-data-version

    Print version of the published Data to the standard output.

.. option:: -q, --quiet

    Turn off all non-error output.

.. option:: -v, --verbose

    Produce verbose output.

.. option:: -V, --version

    Print program version and exit.

Examples
--------

The following command will publish the text of the GPL-3 license under the ``/localhost/demo/gpl3``
prefix::

    ndnputchunks /localhost/demo/gpl3 < /usr/share/common-licenses/GPL-3

To see the published version, you can run the program with the **-p** option::

    ndnputchunks -p /localhost/demo/gpl3 < /usr/share/common-licenses/GPL-3

This command will print the published version to the standard output.

To publish Data with a specific version, you must append a version component to the end of the
prefix. The version component must follow the aforementioned NDN naming conventions.
For example, the following command will publish version 1615519151142 of ``/localhost/demo/gpl3``
using the "typed" naming convention::

    ndnputchunks -Nt /localhost/demo/gpl3/v=1615519151142 < /usr/share/common-licenses/GPL-3

See Also
--------

.. target-notes::
