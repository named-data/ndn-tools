# syntax=docker/dockerfile:1

ARG NDN_CXX_VERSION=latest
FROM ghcr.io/named-data/ndn-cxx-build:${NDN_CXX_VERSION} AS build

RUN apt-get install -Uy --no-install-recommends \
        libpcap-dev \
    && apt-get distclean

ARG JOBS
ARG SOURCE_DATE_EPOCH
RUN --mount=rw,target=/src <<EOF
    set -eux
    cd /src
    ./waf configure \
        --prefix=/usr \
        --libdir=/usr/lib \
        --sysconfdir=/etc \
        --localstatedir=/var \
        --sharedstatedir=/var
    ./waf build
    ./waf install
    mkdir -p /deps/debian
    touch /deps/debian/control
    cd /deps
    for binary in ndnpeek ndnpoke ndncatchunks ndnputchunks ndnping ndnpingserver ndndump ndn-dissect; do
        dpkg-shlibdeps --ignore-missing-info "/usr/bin/${binary}" -O \
            | sed -n 's|^shlibs:Depends=||p' | sed 's| ([^)]*),\?||g' > "${binary}"
    done
EOF


FROM ghcr.io/named-data/ndn-cxx-runtime:${NDN_CXX_VERSION} AS ndn-tools

COPY --link --from=build /usr/bin/ndnpeek /usr/bin/
COPY --link --from=build /usr/bin/ndnpoke /usr/bin/
COPY --link --from=build /usr/bin/ndncatchunks /usr/bin/
COPY --link --from=build /usr/bin/ndnputchunks /usr/bin/
COPY --link --from=build /usr/bin/ndnping /usr/bin/
COPY --link --from=build /usr/bin/ndnpingserver /usr/bin/
COPY --link --from=build /usr/bin/ndndump /usr/bin/
COPY --link --from=build /usr/bin/ndn-dissect /usr/bin/

RUN --mount=from=build,source=/deps,target=/deps \
    apt-get install -Uy --no-install-recommends \
        $(cat /deps/ndn*) \
    && apt-get distclean

ENV HOME=/config
VOLUME /config
VOLUME /run/nfd
