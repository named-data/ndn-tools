# ndn-pib

**ndn-pib** is a service to manage the public information of keys (e.g., identity, public key, and
certificates). It provides a query interface to local applications for lookup and update this
information and also publishes certificates in the network.

Usage example:

1. start NFD on local machine
2. execute `ndn-pib [-h] -o username -d /database/dir -t tpm-locator`

For more information, consult the manpage.
