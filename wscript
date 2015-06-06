# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
import os

VERSION='0.1'
APPNAME='ndn-tools'

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['default-compiler-flags', 'sphinx_build', 'boost'], tooldir=['.waf-tools'])

def configure(conf):
    conf.load(['compiler_cxx', 'gnu_dirs',
               'default-compiler-flags', 'sphinx_build', 'boost'])

    if not os.environ.has_key('PKG_CONFIG_PATH'):
        os.environ['PKG_CONFIG_PATH'] = ':'.join([
            '/usr/lib/pkgconfig',
            '/usr/local/lib/pkgconfig',
            '/opt/local/lib/pkgconfig'])
    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    conf.check_boost(lib='system iostreams regex')

    conf.recurse('tools')

def build(bld):
    bld.env['VERSION'] = VERSION

    bld(
        target='core-objects',
        name='core-objects',
        features='cxx',
        source=bld.path.ant_glob(['core/*.cpp']),
        use='NDN_CXX',
        export_includes='.',
        )

    bld.recurse('tools')
    bld.recurse('manpages')
