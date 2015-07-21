ndn-pib
========

``ndn-pib`` is a public key and certificate management and publishing service.

Usage
-----

::

    ndn-pib [-h] -o owner -d database_dir -t tpm_locator

Description
-----------

This command will start a PIB service process which serves signing key lookup/management for local
applications and also public the public key certificate of signing keys.  The lookup/management
interface listens on a prefix "/localhost/pib/[OwnerName]" and accept five types of command (get,
default, list, update, delete). More details can be found at `Public key Info Base (PIB) Service
<http://redmine.named-data.net/projects/ndn-cxx/wiki/PublicKey_Info_Base>`__

Since PIB service is queried through Interest/Data exchange, NFD is required to run before this
command is executed.

This command requires three arguments: ``owner`` specify the owner of the pib service, which is also
the third name component in the local lookup/management prefix; ``database_dir`` is the path to the
database where all the public key, certificate, identity are stored and managed; ``tpm_locator`` is
locator of a TPM which stores the corresponding private key of the public keys in PIB.

Example
-------

::

    $ ndn-pib -o alice -d /var/pib -t tpm-file:/var/ndn/tpm
