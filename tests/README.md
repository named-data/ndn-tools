ndn-tool unit tests
===================

## Assumptions

Unit tests for a tool `foo` should be placed in the folder `foo` and build script for the tool
should define `foo-objects` that includes all object files for the tool, except object files
defining main function.

For example,

    bld(features='cxx',
        name='tool-subtool-objects',
        source=bld.path.ant_glob('subtool/*.cpp', excl='subtool/main.cpp'),
        use='core-objects')

    bld(features='cxx cxxprogram',
        target='../../bin/subtool',
        source='subtool/main.cpp',
        use='tool-subtool-objects')

    bld(name='tool-objects',
        use='tool-subtool-objects')
