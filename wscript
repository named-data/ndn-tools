# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from waflib import Context, Logs, Utils
import os, subprocess

VERSION = '0.7.1'
APPNAME = 'ndn-tools'
GIT_TAG_PREFIX = 'ndn-tools-'

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['default-compiler-flags',
              'coverage', 'sanitizers', 'boost',
              'sphinx_build'],
             tooldir=['.waf-tools'])

    optgrp = opt.add_option_group('Tools Options')
    optgrp.add_option('--with-tests', action='store_true', default=False,
                      help='Build unit tests')

    opt.recurse('tools')

def configure(conf):
    conf.load(['compiler_cxx', 'gnu_dirs',
               'default-compiler-flags', 'boost',
               'sphinx_build'])

    conf.env.WITH_TESTS = conf.options.with_tests

    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'], uselib_store='NDN_CXX',
                   pkg_config_path=os.environ.get('PKG_CONFIG_PATH', '%s/pkgconfig' % conf.env.LIBDIR))

    boost_libs = ['system', 'program_options', 'filesystem']
    if conf.env.WITH_TESTS:
        boost_libs.append('unit_test_framework')
        conf.define('WITH_TESTS', 1)

    conf.check_boost(lib=boost_libs, mt=True)
    if conf.env.BOOST_VERSION_NUMBER < 105800:
        conf.fatal('The minimum supported version of Boost is 1.65.1.\n'
                   'Please upgrade your distribution or manually install a newer version of Boost.\n'
                   'For more information, see https://redmine.named-data.net/projects/nfd/wiki/Boost')
    elif conf.env.BOOST_VERSION_NUMBER < 106501:
        Logs.warn('WARNING: Using a version of Boost older than 1.65.1 is not officially supported and may not work.\n'
                  'If you encounter any problems, please upgrade your distribution or manually install a newer version of Boost.\n'
                  'For more information, see https://redmine.named-data.net/projects/nfd/wiki/Boost')

    conf.recurse('tools')

    conf.check_compiler_flags()

    # Loading "late" to prevent tests from being compiled with profiling flags
    conf.load('coverage')
    conf.load('sanitizers')

    conf.msg('Tools to build', ', '.join(conf.env.BUILD_TOOLS))

def build(bld):
    version(bld)

    bld(features='subst',
        name='version.cpp',
        source='core/version.cpp.in',
        target='core/version.cpp',
        VERSION_BUILD=VERSION)

    bld.objects(target='core-objects',
                source=bld.path.ant_glob('core/*.cpp') + ['core/version.cpp'],
                use='NDN_CXX BOOST',
                includes='.',
                export_includes='.')

    if Utils.unversioned_sys_platform() == 'linux':
        systemd_units = bld.path.ant_glob('systemd/*.in')
        bld(features='subst',
            name='systemd-units',
            source=systemd_units,
            target=[u.change_ext('') for u in systemd_units])

    bld.recurse('tools')
    bld.recurse('tests')

    if bld.env.SPHINX_BUILD:
        bld(features='sphinx',
            name='manpages',
            builder='man',
            config='manpages/conf.py',
            outdir='manpages',
            source=bld.path.ant_glob('manpages/*.rst'),
            install_path='${MANDIR}',
            version=VERSION_BASE,
            release=VERSION)

def version(ctx):
    # don't execute more than once
    if getattr(Context.g_module, 'VERSION_BASE', None):
        return

    Context.g_module.VERSION_BASE = Context.g_module.VERSION
    Context.g_module.VERSION_SPLIT = VERSION_BASE.split('.')

    # first, try to get a version string from git
    gotVersionFromGit = False
    try:
        cmd = ['git', 'describe', '--always', '--match', '%s*' % GIT_TAG_PREFIX]
        out = subprocess.check_output(cmd, universal_newlines=True).strip()
        if out:
            gotVersionFromGit = True
            if out.startswith(GIT_TAG_PREFIX):
                Context.g_module.VERSION = out.lstrip(GIT_TAG_PREFIX)
            else:
                # no tags matched
                Context.g_module.VERSION = '%s-commit-%s' % (VERSION_BASE, out)
    except (OSError, subprocess.CalledProcessError):
        pass

    versionFile = ctx.path.find_node('VERSION.info')
    if not gotVersionFromGit and versionFile is not None:
        try:
            Context.g_module.VERSION = versionFile.read()
            return
        except EnvironmentError:
            pass

    # version was obtained from git, update VERSION file if necessary
    if versionFile is not None:
        try:
            if versionFile.read() == Context.g_module.VERSION:
                # already up-to-date
                return
        except EnvironmentError as e:
            Logs.warn('%s exists but is not readable (%s)' % (versionFile, e.strerror))
    else:
        versionFile = ctx.path.make_node('VERSION.info')

    try:
        versionFile.write(Context.g_module.VERSION)
    except EnvironmentError as e:
        Logs.warn('%s is not writable (%s)' % (versionFile, e.strerror))

def dist(ctx):
    version(ctx)

def distcheck(ctx):
    version(ctx)
