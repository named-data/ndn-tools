# ndnserve

**ndnserve** is a producer program that reads a file from the standard input, and makes it
available as a set of NDN Data segments. It appends version and segment number components
to the specified name as needed, according to the [NDN naming conventions](
https://named-data.net/publications/techreports/ndn-tr-22-3-ndn-memo-naming-conventions/).

Files published by ndnserve can be fetched with [ndnget](../get/README.md).

## Usage examples

The following command will publish the text of the GPL-3 license under the `/localhost/demo/gpl3`
prefix:

    ndnserve /localhost/demo/gpl3 < /usr/share/common-licenses/GPL-3

To find the published version you have to start ndnserve with the `-p` command line option,
for example:

    ndnserve -p /localhost/demo/gpl3 < /usr/share/common-licenses/GPL-3

This command will print the published version to standard output.

To publish Data with a specific version, you need to append a version component to the end of the
prefix. The version component must follow the aforementioned NDN naming conventions. For example,
the following command will publish the version 1449078495094 of the `/localhost/demo/gpl3` prefix:

    ndnserve -Nt /localhost/demo/gpl3/v=1449078495094 < /usr/share/common-licenses/GPL-3

If the specified version component is not valid, ndnserve will exit with an error. If no version
component is specified, one will be generated and appended to the name.
