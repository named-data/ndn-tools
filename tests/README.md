# ndn-tools unit tests

## Assumptions

Unit tests for a tool `foo` should be placed in the folder `foo` and the build script
for the tool should define `foo-objects` that includes all object files for that tool,
except the object that contains the `main()` function.

For example:

```python
def build(bld):
    bld.objects(
        target='tool-subtool-objects',
        source=bld.path.ant_glob('subtool/*.cpp', excl='subtool/main.cpp'),
        use='core-objects')

    bld.program(
        name='subtool',
        target='../../bin/subtool',
        source='subtool/main.cpp',
        use='tool-subtool-objects')

    bld(name='tool-objects',
        use='tool-subtool-objects')
```
