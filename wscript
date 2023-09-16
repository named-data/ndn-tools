# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os
import subprocess
from waflib import Context, Logs, Utils

VERSION = '22.12'
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

    # Prefer pkgconf if it's installed, because it gives more correct results
    # on Fedora/CentOS/RHEL/etc. See https://bugzilla.redhat.com/show_bug.cgi?id=1953348
    # Store the result in env.PKGCONFIG, which is the variable used inside check_cfg()
    conf.find_program(['pkgconf', 'pkg-config'], var='PKGCONFIG')

    pkg_config_path = os.environ.get('PKG_CONFIG_PATH', f'{conf.env.LIBDIR}/pkgconfig')
    conf.check_cfg(package='libndn-cxx', args=['libndn-cxx >= 0.8.1', '--cflags', '--libs'],
                   uselib_store='NDN_CXX', pkg_config_path=pkg_config_path)

    conf.check_boost(lib='program_options', mt=True)
    if conf.env.BOOST_VERSION_NUMBER < 107100:
        conf.fatal('The minimum supported version of Boost is 1.71.0.\n'
                   'Please upgrade your distribution or manually install a newer version of Boost.\n'
                   'For more information, see https://redmine.named-data.net/projects/nfd/wiki/Boost')

    if conf.env.WITH_TESTS:
        conf.check_boost(lib='unit_test_framework', mt=True, uselib_store='BOOST_TESTS')

    conf.recurse('tools')

    conf.check_compiler_flags()

    # Loading "late" to prevent tests from being compiled with profiling flags
    conf.load('coverage')
    conf.load('sanitizers')

    conf.define_cond('WITH_TESTS', conf.env.WITH_TESTS)

    conf.msg('Tools to build', ', '.join(conf.env.BUILD_TOOLS))

def build(bld):
    version(bld)

    bld(features='subst',
        name='version.cpp',
        source='core/version.cpp.in',
        target='core/version.cpp',
        VERSION_BUILD=VERSION)

    bld.objects(
        target='core-objects',
        source=bld.path.find_dir('core').ant_glob('*.cpp') + ['core/version.cpp'],
        use='BOOST NDN_CXX',
        includes='.',
        export_includes='.')

    bld.recurse('tools')

    if bld.env.WITH_TESTS:
        bld.recurse('tests')

    if Utils.unversioned_sys_platform() == 'linux':
        systemd_units = bld.path.ant_glob('systemd/*.in')
        bld(features='subst',
            name='systemd-units',
            source=systemd_units,
            target=[u.change_ext('') for u in systemd_units])

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
        cmd = ['git', 'describe', '--always', '--match', f'{GIT_TAG_PREFIX}*']
        out = subprocess.run(cmd, capture_output=True, check=True, text=True).stdout.strip()
        if out:
            gotVersionFromGit = True
            if out.startswith(GIT_TAG_PREFIX):
                Context.g_module.VERSION = out.lstrip(GIT_TAG_PREFIX)
            else:
                # no tags matched
                Context.g_module.VERSION = f'{VERSION_BASE}-commit-{out}'
    except (OSError, subprocess.SubprocessError):
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
            Logs.warn(f'{versionFile} exists but is not readable ({e.strerror})')
    else:
        versionFile = ctx.path.make_node('VERSION.info')

    try:
        versionFile.write(Context.g_module.VERSION)
    except EnvironmentError as e:
        Logs.warn(f'{versionFile} is not writable ({e.strerror})')

def dist(ctx):
    ctx.algo = 'tar.xz'
    version(ctx)

def distcheck(ctx):
    ctx.algo = 'tar.xz'
    version(ctx)
