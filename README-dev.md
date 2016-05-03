# Notes for ndn-tools Developers

## Licensing Requirements

Contributions to ndn-tools MUST be licensed under GPL 3.0 or a compatible license.  
If you choose GPL 3.0, include the following license boilerplate into all C++ code files:

    /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
    /**
     * Copyright (c) [Year(s)],  [Copyright Holder(s)].
     *
     * This file is part of ndn-tools (Named Data Networking Essential Tools).
     * See AUTHORS.md for complete list of ndn-tools authors and contributors.
     *
     * ndn-tools is free software: you can redistribute it and/or modify it under the terms
     * of the GNU General Public License as published by the Free Software Foundation,
     * either version 3 of the License, or (at your option) any later version.
     *
     * ndn-tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
     * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     * PURPOSE.  See the GNU General Public License for more details.
     *
     * You should have received a copy of the GNU General Public License along with
     * ndn-tools, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
     */

## Directory Structure and Build Script

All tools are placed in subdirectories under `tools/` directory.

A tool can consist of one or more programs.
For instance, a pair of consumer and producer programs that are designed to work together
should be considered a single tool, not two separate tools.

Each tool MUST have a `wscript` build script in its subdirectory.
It will be invoked if this tool is selected for the build.
It SHOULD compile the programs into `build/bin` directory (`target='../../bin/foo'`).

### Shared Modules

Modules shared among multiple tools SHOULD be placed in `core/` directory.
They are available for use in all tools.

A header in `core/` can be included in a tool like `#include "core/foo.hpp"`.

`wscript` of a tool can link a program with modules in `core/` with `use='core-objects'`.

### Documentation

`README.md` in the subdirectory of a tool SHOULD give a brief description.

Manual pages for each program SHOULD be written in reStructuredText format
and placed in `manpages/` directory.

## Code Guidelines

C++ code SHOULD conform to
[ndn-cxx code style](http://named-data.net/doc/ndn-cxx/current/code-style.html).

### Namespace

Types in each tool SHOULD be declared in a sub-namespace under `namespace ndn`.
For example, a tool in `tools/foo` directory has namespace `ndn::foo`.  
This allows the tool to reference ndn-cxx types with unqualified name lookup.
This also prevents name conflicts between ndn-cxx and tools.

Types in `core/` SHOULD be declared directly under `namespace ndn`,
or in a sub-namespace if desired.

`using namespace` SHOULD NOT be used except within block scope.

### main Function

The main function of a program SHOULD be declared within its sub-namespace.
This allows it to reference types in ndn-cxx and the tool with unqualified name lookup.

Then, another main function in global namespace needs to be defined
to call the main function in sub-namespace.

For example:

    namespace ndn {
    namespace foo {
    
    class Bar
    {
    public:
      explicit
      Bar(Face& face);
      
      void
      run();
    };
    
    int
    main(int argc, char** argv)
    {
      Face face;
      Bar program(face);
      program.run();
      return 0;
    }
    
    } // namespace foo
    } // namespace ndn
    
    int
    main(int argc, char** argv)
    {
      return ndn::foo::main(argc, argv);
    }

### Command Line Arguments

[Boost.Program\_options](http://www.boost.org/doc/libs/1_54_0/doc/html/program_options.html) is
preferred over getopt(3) for parsing command line arguments.
