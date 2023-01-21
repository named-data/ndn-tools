# ndncatchunks and ndnputchunks

**ndncatchunks** and **ndnputchunks** are a pair of programs to transfer a file as Data segments.

* **ndnputchunks** is a producer program that reads a file from the standard input, and makes
  it available as a set of NDN Data segments. It appends version and segment number components
  to the specified name as needed, according to the [NDN naming conventions](
  https://named-data.net/publications/techreports/ndn-tr-22-3-ndn-memo-naming-conventions/).

* **ndncatchunks** is a consumer program that fetches Data segments of a file, optionally
  discovering the latest version of the file, and writes the content of the retrieved file to
  the standard output.

## Version discovery in ndncatchunks

If a version component is present at the end of the user-specified NDN name, the provided version
number will be used, without any version discovery process. Otherwise, discovery Interest(s) will
be sent out to fetch metadata of the solicited content from which the Data version will be resolved.
For more information about the packet format and naming conventions of Interest and Data packets
used for version discovery in ndncatchunks, please refer to:
[Realtime Data Retrieval (RDR) protocol](https://redmine.named-data.net/projects/ndn-tlv/wiki/RDR).

## Interest pipeline types in ndncatchunks

* `fixed`: maintains a fixed-size window of Interests in flight; the window size is configurable
           via a command line option and defaults to 1.

* `aimd` : adjusts the window size via additive-increase/multiplicative-decrease (AIMD).
           By default, it uses a Conservative Window Adaptation, that is, the congestion window
           will be decreased at most once per round-trip-time.

* `cubic`: adjusts the window size similar to the TCP CUBIC algorithm.
           For details about both aimd and cubic please refer to:
           [A Practical Congestion Control Scheme for Named Data
           Networking](https://conferences2.sigcomm.org/acm-icn/2016/proceedings/p21-schneider.pdf).

The default Interest pipeline type is `cubic`.

## Usage examples

### Publishing

The following command will publish the text of the GPL-3 license under the `/localhost/demo/gpl3`
prefix:

    ndnputchunks /localhost/demo/gpl3 < /usr/share/common-licenses/GPL-3

To find the published version you have to start ndnputchunks with the `-p` command line option,
for example:

    ndnputchunks -p /localhost/demo/gpl3 < /usr/share/common-licenses/GPL-3

This command will print the published version to standard output.

To publish Data with a specific version, you need to append a version component to the end of the
prefix. The version component must follow the aforementioned NDN naming conventions. For example,
the following command will publish the version 1449078495094 of the `/localhost/demo/gpl3` prefix:

    ndnputchunks -Nt /localhost/demo/gpl3/v=1449078495094 < /usr/share/common-licenses/GPL-3

If the specified version component is not valid, ndnputchunks will exit with an error. If no version
component is specified, one will be generated and appended to the name.

### Retrieval

To retrieve the latest version of a published file, the following command can be used:

    ndncatchunks /localhost/demo/gpl3

To fetch a specific version of a published file, you can specify the version number at the end of
the name. For example, if the version is known to be 1449078495094, the following command
will fetch that exact version of the file (without version discovery):

    ndncatchunks -Nt /localhost/demo/gpl3/v=1449078495094

For more information, run the programs with `--help` as argument.
