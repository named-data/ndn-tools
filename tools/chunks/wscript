# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
top = '../..'

def build(bld):

    bld(features='cxx',
        name='ndncatchunks-objects',
        source=bld.path.ant_glob('catchunks/*.cpp', excl='catchunks/ndncatchunks.cpp'),
        use='core-objects')

    bld(features='cxx cxxprogram',
        target='../../bin/ndncatchunks',
        source='catchunks/ndncatchunks.cpp',
        use='ndncatchunks-objects')

    bld(features='cxx',
        name='ndnputchunks-objects',
        source=bld.path.ant_glob('putchunks/*.cpp', excl='putchunks/ndnputchunks.cpp'),
        use='core-objects')

    bld(features='cxx cxxprogram',
        target='../../bin/ndnputchunks',
        source='putchunks/ndnputchunks.cpp',
        use='ndnputchunks-objects')

    ## (for unit tests)

    bld(name='chunks-objects',
        use='ndncatchunks-objects ndnputchunks-objects')

