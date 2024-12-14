import platform
from waflib import Configure, Logs, Utils


def options(opt):
    opt.add_option('--debug', '--with-debug', action='store_true', default=False,
                   help='Compile in debugging mode with minimal optimizations (-Og)')


def configure(conf):
    conf.start_msg('Checking C++ compiler version')

    cxx = conf.env.CXX_NAME # generic name of the compiler
    ccver = tuple(int(i) for i in conf.env.CC_VERSION)
    ccverstr = '.'.join(conf.env.CC_VERSION)
    errmsg = ''
    warnmsg = ''
    if cxx == 'gcc':
        if ccver < (9, 1, 0):
            errmsg = ('The version of gcc you are using is too old.\n'
                      'The minimum supported gcc version is 10.2.')
        elif ccver < (10, 2, 0):
            warnmsg = ('Using a version of gcc older than 10.2 is not '
                       'officially supported and may result in build failures.')
        conf.flags = GccFlags()
    elif cxx == 'clang':
        if Utils.unversioned_sys_platform() == 'darwin':
            if ccver < (11, 0, 0):
                errmsg = ('The version of Xcode you are using is too old.\n'
                          'The minimum supported Xcode version is 13.0.')
            elif ccver < (13, 0, 0):
                warnmsg = ('Using a version of Xcode older than 13.0 is not '
                           'officially supported and may result in build failures.')
        elif ccver < (7, 0, 0):
            errmsg = ('The version of clang you are using is too old.\n'
                      'The minimum supported clang version is 10.0.')
        elif ccver < (10, 0, 0):
            warnmsg = ('Using a version of clang older than 10.0 is not '
                       'officially supported and may result in build failures.')
        conf.flags = ClangFlags()
    else:
        warnmsg = f'{cxx} compiler is unsupported'
        conf.flags = CompilerFlags()

    if errmsg:
        conf.end_msg(ccverstr, color='RED')
        conf.fatal(errmsg)
    elif warnmsg:
        conf.end_msg(ccverstr, color='YELLOW')
        Logs.warn('WARNING: ' + warnmsg)
    else:
        conf.end_msg(ccverstr)

    conf.areCustomCxxflagsPresent = len(conf.env.CXXFLAGS) > 0

    # General flags are always applied (e.g., selecting C++ language standard)
    generalFlags = conf.flags.getGeneralFlags(conf)
    conf.add_supported_cxxflags(generalFlags['CXXFLAGS'])
    conf.add_supported_linkflags(generalFlags['LINKFLAGS'])
    conf.env.DEFINES += generalFlags['DEFINES']


@Configure.conf
def check_compiler_flags(conf):
    # Debug or optimized CXXFLAGS and LINKFLAGS are applied only if the
    # corresponding environment variables are not set.
    # DEFINES are always applied.
    if conf.options.debug:
        extraFlags = conf.flags.getDebugFlags(conf)
        if conf.areCustomCxxflagsPresent:
            missingFlags = [x for x in extraFlags['CXXFLAGS'] if x not in conf.env.CXXFLAGS]
            if missingFlags:
                Logs.warn('Selected debug mode, but CXXFLAGS is set to a custom value "%s"'
                          % ' '.join(conf.env.CXXFLAGS))
                Logs.warn('Default flags "%s" will not be used' % ' '.join(missingFlags))
    else:
        extraFlags = conf.flags.getOptimizedFlags(conf)

    if not conf.areCustomCxxflagsPresent:
        conf.add_supported_cxxflags(extraFlags['CXXFLAGS'])
        conf.add_supported_linkflags(extraFlags['LINKFLAGS'])

    conf.env.DEFINES += extraFlags['DEFINES']


@Configure.conf
def add_supported_cxxflags(self, cxxflags):
    """
    Check which cxxflags are supported by the active compiler and add them to env.CXXFLAGS variable.
    """
    if len(cxxflags) == 0:
        return

    self.start_msg('Checking supported CXXFLAGS')

    supportedFlags = []
    for flags in cxxflags:
        flags = Utils.to_list(flags)
        if self.check_cxx(cxxflags=['-Werror'] + flags, mandatory=False):
            supportedFlags += flags

    self.end_msg(' '.join(supportedFlags))
    self.env.prepend_value('CXXFLAGS', supportedFlags)


@Configure.conf
def add_supported_linkflags(self, linkflags):
    """
    Check which linkflags are supported by the active compiler and add them to env.LINKFLAGS variable.
    """
    if len(linkflags) == 0:
        return

    self.start_msg('Checking supported LINKFLAGS')

    supportedFlags = []
    for flags in linkflags:
        flags = Utils.to_list(flags)
        if self.check_cxx(linkflags=['-Werror'] + flags, mandatory=False):
            supportedFlags += flags

    self.end_msg(' '.join(supportedFlags))
    self.env.prepend_value('LINKFLAGS', supportedFlags)


class CompilerFlags:
    def getCompilerVersion(self, conf):
        return tuple(int(i) for i in conf.env.CC_VERSION)

    def getGeneralFlags(self, conf):
        """Get dict of CXXFLAGS, LINKFLAGS, and DEFINES that are always needed"""
        return {'CXXFLAGS': [], 'LINKFLAGS': [], 'DEFINES': []}

    def getDebugFlags(self, conf):
        """Get dict of CXXFLAGS, LINKFLAGS, and DEFINES that are needed only in debug mode"""
        return {
            'CXXFLAGS': [],
            'LINKFLAGS': [],
            'DEFINES': ['BOOST_ASIO_NO_DEPRECATED'],
        }

    def getOptimizedFlags(self, conf):
        """Get dict of CXXFLAGS, LINKFLAGS, and DEFINES that are needed only in optimized mode"""
        return {'CXXFLAGS': [], 'LINKFLAGS': [], 'DEFINES': ['NDEBUG']}


class GccClangCommonFlags(CompilerFlags):
    """
    This class defines common flags that work for both gcc and clang compilers.
    """

    def getGeneralFlags(self, conf):
        flags = super().getGeneralFlags(conf)
        flags['CXXFLAGS'] += ['-std=c++17']
        if Utils.unversioned_sys_platform() != 'darwin':
            flags['LINKFLAGS'] += ['-fuse-ld=lld']
        return flags

    __cxxFlags = [
        '-fdiagnostics-color',
        '-Wall',
        '-Wextra',
        '-Wpedantic',
        '-Wenum-conversion',
        '-Wextra-semi',
        '-Wno-unused-parameter',
    ]
    __linkFlags = ['-Wl,-O1']

    def getDebugFlags(self, conf):
        flags = super().getDebugFlags(conf)
        flags['CXXFLAGS'] += ['-Og', '-g'] + self.__cxxFlags + [
            '-Werror',
            '-Wno-error=deprecated-declarations', # Bug #3795
            '-Wno-error=maybe-uninitialized', # Bug #1615
        ]
        flags['LINKFLAGS'] += self.__linkFlags
        # Enable assertions in libstdc++
        # https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_macros.html
        flags['DEFINES'] += ['_GLIBCXX_ASSERTIONS=1']
        return flags

    def getOptimizedFlags(self, conf):
        flags = super().getOptimizedFlags(conf)
        flags['CXXFLAGS'] += ['-O2', '-g1'] + self.__cxxFlags
        flags['LINKFLAGS'] += self.__linkFlags
        return flags


class GccFlags(GccClangCommonFlags):
    __cxxFlags = [
        '-Wcatch-value=2',
        '-Wcomma-subscript', # enabled by default in C++20
        '-Wduplicated-branches',
        '-Wduplicated-cond',
        '-Wlogical-op',
        '-Wredundant-tags',
        '-Wvolatile', # enabled by default in C++20
    ]

    def getDebugFlags(self, conf):
        flags = super().getDebugFlags(conf)
        flags['CXXFLAGS'] += self.__cxxFlags
        if platform.machine() == 'armv7l':
            flags['CXXFLAGS'] += ['-Wno-psabi'] # Bug #5106
        return flags

    def getOptimizedFlags(self, conf):
        flags = super().getOptimizedFlags(conf)
        flags['CXXFLAGS'] += self.__cxxFlags
        if platform.machine() == 'armv7l':
            flags['CXXFLAGS'] += ['-Wno-psabi'] # Bug #5106
        return flags


class ClangFlags(GccClangCommonFlags):
    def getGeneralFlags(self, conf):
        flags = super().getGeneralFlags(conf)
        if Utils.unversioned_sys_platform() == 'darwin':
            # Bug #4296
            brewdir = '/opt/homebrew' if platform.machine() == 'arm64' else '/usr/local'
            flags['CXXFLAGS'] += [
                ['-isystem', f'{brewdir}/include'], # for Homebrew
                ['-isystem', '/opt/local/include'], # for MacPorts
            ]
        elif Utils.unversioned_sys_platform() == 'freebsd':
            # Bug #4790
            flags['CXXFLAGS'] += [['-isystem', '/usr/local/include']]
        if self.getCompilerVersion(conf) >= (18, 0, 0):
            # Bug #5300
            flags['CXXFLAGS'] += ['-Wno-enum-constexpr-conversion']
        return flags

    __cxxFlags = [
        '-Wundefined-func-template',
        '-Wno-unused-local-typedef', # Bugs #2657 and #3209
    ]

    def getDebugFlags(self, conf):
        flags = super().getDebugFlags(conf)
        flags['CXXFLAGS'] += self.__cxxFlags
        # Enable assertions in libc++
        if self.getCompilerVersion(conf) >= (18, 0, 0):
            # https://libcxx.llvm.org/Hardening.html
            flags['DEFINES'] += ['_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE']
        elif self.getCompilerVersion(conf) >= (15, 0, 0):
            # https://releases.llvm.org/15.0.0/projects/libcxx/docs/UsingLibcxx.html#enabling-the-safe-libc-mode
            flags['DEFINES'] += ['_LIBCPP_ENABLE_ASSERTIONS=1']
        # Tell libc++ to avoid including transitive headers
        # https://libcxx.llvm.org/DesignDocs/HeaderRemovalPolicy.html
        flags['DEFINES'] += ['_LIBCPP_REMOVE_TRANSITIVE_INCLUDES=1']
        return flags

    def getOptimizedFlags(self, conf):
        flags = super().getOptimizedFlags(conf)
        flags['CXXFLAGS'] += self.__cxxFlags
        return flags
