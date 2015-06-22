# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
top = '../..'

def build(bld):

    bld(features='cxx',
        name='ping-client-objects',
        source=bld.path.ant_glob('client/*.cpp', excl='client/ndn-ping.cpp'),
        use='core-objects')

    bld(features='cxx cxxprogram',
        target='../../bin/ndnping',
        source='client/ndn-ping.cpp',
        use='ping-client-objects')

    bld(features='cxx',
        name='ping-server-objects',
        source=bld.path.ant_glob('server/*.cpp', excl='server/ndn-ping-server.cpp'),
        use='core-objects')

    bld(features='cxx cxxprogram',
        target='../../bin/ndnpingserver',
        source='server/ndn-ping-server.cpp',
        use='ping-server-objects')

    ## (for unit tests)

    bld(name='ping-objects',
        use='ping-client-objects ping-server-objects')
