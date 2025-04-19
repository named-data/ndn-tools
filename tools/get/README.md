# ndnget

**ndnget** is a consumer program that fetches the segments (Data packets) of a named object,
optionally discovering the latest version of the object, and then writes the content of the
retrieved object to the standard output.

## Version discovery in ndnget

If a version component is present at the end of the user-specified NDN name, the provided version
number will be used, without any version discovery process. Otherwise, discovery Interest(s) will
be sent out to fetch metadata of the solicited content from which the Data version will be resolved.
For more information about the packet format and naming conventions of Interest and Data packets
used for version discovery in ndnget, please refer to:
[Realtime Data Retrieval (RDR) protocol](https://redmine.named-data.net/projects/ndn-tlv/wiki/RDR).

## Interest pipeline types in ndnget

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

To retrieve the latest version of a published object, the following command can be used:

    ndnget /localhost/demo/gpl3

To fetch a specific version of a published object, you can specify the version number at the end
of the name. For example, if the version is known to be 1449078495094, the following command
will fetch that exact version of the object (without version discovery):

    ndnget -Nt /localhost/demo/gpl3/v=1449078495094

For more information, run the programs with `--help` as argument.
