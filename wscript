# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

VERSION = '0.3'
APPNAME = 'ndn-tools'

from waflib import Utils
import os

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['default-compiler-flags', 'sanitizers', 'sphinx_build', 'boost'],
             tooldir=['.waf-tools'])

    opt.add_option('--with-tests', action='store_true', default=False,
                   dest='with_tests', help='''Build unit tests''')

    opt.recurse('tools')

def configure(conf):
    conf.load(['compiler_cxx', 'gnu_dirs',
               'default-compiler-flags', 'sanitizers', 'sphinx_build', 'boost'])

    if 'PKG_CONFIG_PATH' not in os.environ:
        os.environ['PKG_CONFIG_PATH'] = Utils.subst_vars('${LIBDIR}/pkgconfig', conf.env)
    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    boost_libs = 'system iostreams regex'
    if conf.options.with_tests:
        conf.env['WITH_TESTS'] = 1
        conf.define('WITH_TESTS', 1);
        boost_libs += ' unit_test_framework'
    conf.check_boost(lib=boost_libs)

    conf.recurse('tools')

def build(bld):
    bld.env['VERSION'] = VERSION

    bld(target='core-objects',
        name='core-objects',
        features='cxx',
        source=bld.path.ant_glob(['core/*.cpp']),
        use='NDN_CXX BOOST',
        export_includes='.')

    bld.recurse('tools')
    bld.recurse('tests')
    bld.recurse('manpages')
