MKVToolNix 13.0.0
================

# Table of contents

1. [Introduction](#1-introduction)
2. [Installation](#2-installation)
  1. [Requirements](#21-requirements)
  2. [Optional components](#22-optional-components)
  3. [Building libEBML and libMatroska](#23-building-libebml-and-libmatroska)
  4. [Building MKVToolNix](#24-building-mkvtoolnix)
    1. [Getting and building a development snapshot](#241-getting-and-building-a-development-snapshot)
    2. [Configuration and compilation](#242-configuration-and-compilation)
  5. [Notes for compilation on (Open)Solaris](#25-notes-for-compilation-on-opensolaris)
  6. [Unit tests](#26-unit-tests)
3. [Reporting bugs](#3-reporting-bugs)
4. [Test suite and continuous integration tests](#4-test-suite-and-continuous-integration-tests)
5. [Included libraries and their licenses](#5-included-libraries-and-their-licenses)
  1. [avilib](#51-avilib)
  2. [Boost's utf8_codecvt_facet](#52-boosts-utf8_codecvt_facet)
  3. [libEBML](#53-libebml)
  4. [libMatroska](#54-libmatroska)
  5. [librmff](#55-librmff)
  6. [nlohmann's JSON](#56-nlohmanns-json)
  7. [pugixml](#57-pugixml)
  8. [utf8-cpp](#58-utf8-cpp)

-----------------

# 1. Introduction

With these tools one can get information about (via mkvinfo) Matroska
files, extract tracks/data from (via mkvextract) Matroska files and create
(via mkvmerge) Matroska files from other media files. Matroska is a new
multimedia file format aiming to become THE new container format for
the future. You can find more information about it and its underlying
technology, the Extensible Binary Meta Language (EBML), at

http://www.matroska.org/

The full documentation for each command is now maintained in its
man page only. Type `mkvmerge -h` to get you started.

This code comes under the GPL v2 (see www.gnu.org or the file COPYING).
Modify as needed.

The icons are based on the work of Alexandr Grigorcea and modified by
Eduard Geier. They're licensed under the terms of the
[Creative Commons Attribution 3.0 Unported license](http://creativecommons.org/licenses/by/3.0/).

The newest version can always be found at
https://mkvtoolnix.download/

Moritz Bunkus <moritz@bunkus.org>


# 2. Installation

If you want to compile the tools yourself, you must first decide
if you want to use a 'proper' release version or the current
development version. As both Matroska and MKVToolNix are under heavy
development, there might be features available in the git repository
that are not available in the releases. On the other hand the git
repository version might not even compile.

## 2.1. Requirements

In order to compile MKVToolNix, you need a couple of libraries. Most of
them should be available pre-compiled for your distribution. The
programs and libraries you absolutely need are:

- A C++ compiler that supports several features of the C++11 and C++14
  standards: initializer lists, range-based `for` loops, right angle
  brackets, the `auto` keyword, lambda functions, the `nullptr` keyword, 
  tuples, alias declarations, `std::make_unique()`, digit
  separators, binary literals and generic lambdas. Others may be
  needed, too. For GCC this means at least v4.9.x; for clang v3.4 or
  later.

- [libEBML v1.3.4](http://dl.matroska.org/downloads/libebml/) or later
  and [libMatroska v1.4.5](http://dl.matroska.org/downloads/libmatroska/)
  or later for low-level access to Matroska files. Instructions on how to
  compile them are a bit further down in this file.

- [libOgg](http://downloads.xiph.org/releases/ogg/) and
  [libVorbis](http://downloads.xiph.org/releases/vorbis/) for access to Ogg/OGM
  files and Vorbis support

- [zlib](http://www.zlib.net/) — a compression library

- [Boost](http://www.boost.org/) — Several of Boost's libraries are
  used: `format`, `RegEx`, `filesystem`, `system`, `math`,
  `Range`, `rational`, `variant`. At least v1.46.0 is required.

- [libxslt's xsltproc binary](http://xmlsoft.org/libxslt/) and
  [DocBook XSL stylesheets](https://sourceforge.net/projects/docbook/files/docbook-xsl/)
  — for creating man pages from XML documents

You also need the `rake` or `drake` build program. I suggest `rake`
v10.0.0 or newer (this is included with Ruby 2.1) as it offers
parallel builds out of the box. If you only have an earlier version of
`rake`, you can install and use the `drake` gem for the same gain.

## 2.2. Optional components

Other libraries are optional and only limit the features that are
built. These include:

- [Qt](http://www.qt.io/) v5.3 or newer — a cross-platform GUI
  toolkit. You need this if you want to use the MKVToolNix GUI or
  mkvinfo's GUI.

- [libFLAC](http://downloads.xiph.org/releases/flac/) for FLAC
  support (Free Lossless Audio Codec)

- [lzo](http://www.oberhumer.com/opensource/lzo/) and
  [bzip2](http://www.bzip.org/) are compression libraries. These are
  the least important libraries as almost no application supports
  Matroska content that is compressed with either of these libs. The
  aforementioned zlib is what every program supports.

- [libMagic](http://www.darwinsys.com/file/) from the "file" package
  for automatic content type detection

- [po4a](https://po4a.alioth.debian.org/) for building the translated
  man pages

## 2.3. Building libEBML and libMatroska

This is optional as MKVToolNix comes with its own set of the
libraries. It will use them if no version is found on the system.

Start with the two libraries. Either download releases of
[libEBML 1.3.4](http://dl.matroska.org/downloads/libebml/) and
[libMatroska 1.4.5](http://dl.matroska.org/downloads/libmatroska/) or
get a fresh copy from the git repository:

    git clone https://github.com/Matroska-Org/libebml.git
    git clone https://github.com/Matroska-Org/libmatroska.git

First change to libEBML's directory and run `./configure` followed by
`make`. Now install libEBML by running `make install` as root
(e.g. via `sudo`). Change to libMatroska's directory and go through
the same steps: first `./configure` followed by `make` as a normal
user and lastly `make install` as root.

## 2.4. Building MKVToolNix

Either download the current release from
[the MKVToolNix home page](https://mkvtoolnix.download/)
and unpack it or get a development snapshot from my Git repository.

### 2.4.1. Getting and building a development snapshot

You can ignore this subsection if you want to build from a release
tarball.

All you need for Git repository access is to download a Git client
from the Git homepage at http://git-scm.com/. There are clients
for both Unix/Linux and Windows.

First clone my Git repository with this command:

    git clone https://github.com/mbunkus/mkvtoolnix.git

Now change to the MKVToolNix directory with `cd mkvtoolnix` and run
`./autogen.sh` which will generate the "configure" script. You need
the GNU "autoconf" utility for this step.

### 2.4.2. Configuration and compilation

If you have run `make install` for both libraries, then `configure`
should automatically find the libraries' position. Otherwise you need
to tell `configure` where the libEBML and libMatroska include and
library files are:

    ./configure \
      --with-extra-includes=/where/i/put/libebml\;/where/i/put/libmatroska \
      --with-extra-libs=/where/i/put/libebml/make/linux\;/where/i/put/libmatroska/make/linux

Now run `rake` and, as "root", `rake install`.

## 2.5. Notes for compilation on (Open)Solaris

You can compile MKVToolNix with Sun's sunstudio compiler, but you need
additional options for `configure`:

    ./configure --prefix=/usr \
      CXX="/opt/sunstudio12.1/bin/CC -library=stlport4" \
      CXXFLAGS="-D_POSIX_PTHREAD_SEMANTICS" \
      --with-extra-includes=/where/i/put/libebml\;/where/i/put/libmatroska \
      --with-extra-libs=/where/i/put/libebml/make/linux\;/where/i/put/libmatroska/make/linux

## 2.6. Unit tests

Building and running unit tests is completely optional. If you want to
do this, you have to follow these steps:

1. Download the "googletest" framework from
   https://github.com/google/googletest/ (at the time of writing the
   file to download was "googletest-release-1.8.0.tar.gz")

2. Extract the archive somewhere and create a symbolic link to its
   `googletest-release-1.8.0/googletest/include/gtest` sub-directory
   inside MKVToolNix' "lib" directory.

3. Configure MKVToolNix normally.

4. Build the unit test executable and run it with

        rake tests:unit


# 3. Reporting bugs

If you're sure you've found a bug — e.g. if one of my programs crashes
with an obscur error message, or if the resulting file is missing part
of the original data, then by all means submit a bug report.

I use [GitHub's issue system](https://github.com/mbunkus/mkvtoolnix/issues)
as my bug database. You can submit your bug reports there. Please be as
verbose as possible — e.g. include the command line, if you use Windows
or Linux etc.pp.

If at all possible, please include sample files as well so that I can
reproduce the issue. If they are larger than 1 MB, please upload
them somewhere and post a link in the issue. You can also upload them
to my FTP server. Details on how to connect can be found in the
[MKVToolNix FAQ](https://github.com/mbunkus/mkvtoolnix/wiki/FTP-server).

# 4. Test suite and continuous integration tests

MKVToolNix contains a lot of test cases in order to detect regressions
before they're released. Regressions include both compilation issues
as well as changes from expected program behavior.

As mentioned in section 2.6., MKVToolNix comes with a set of unit
tests based on the Google Test library in the `tests/unit`
sub-directory that you can run yourself. These cover only a small
amount of code, and any effort to extend them would be most welcome.

A second test suite exists that targets the program behavior, e.g. the
output generated by mkvmerge when specific options are used with
specific input files. These are the test cases in the `tests`
directory itself. Unfortunately the files they run on often contain
copyrighted material that I cannot distribute. Therefore you cannot
run them yourself.

A third pillar of the testing effort is the
[continuous integration tests](https://buildbot.mkvtoolnix.download/)
run on a Buildbot instance. These are run automatically for each
commit made to the git repository. The tests include:

  * building of all the packages for Linux distributions that I
    normally provide for download myself in both 32-bit and 64-bit
    variants
  * building of the Windows installer and portable packages in both
    32-bit and 64-bit variants
  * building with both g++ and clang++
  * building and running the unit tests
  * building and running the test file test suite
  * building with all optional features disabled

# 5. Included third-party components and their licenses

MKVToolNix includes and uses the following libraries & artwork:

## 5.1. avilib

Reading and writing avi files.

Copyright (C) 1999 Rainer Johanni <Rainer@Johanni.de>, originally part
of the transcode package.

  * License: GNU General Public License v2
  * URL: http://www.transcoding.org/

## 5.2. Boost's utf8_codecvt_facet

A class, `utf8_codecvt_facet`, derived from `std::codecvt<wchar_t, char>`,
which can be used to convert utf8 data in files into `wchar_t` strings
in the application.

  * License: Boost Software License - Version 1.0 (see `doc/licenses/Boost-1.0.txt`)
  * URL: http://www.boost.org

## 5.3. libEBML

A C++ library to parse EBML files

  * License: GNU Lesser General Public License v2.1 (see `doc/licenses/LGPL-2.1.txt`)
  * URL: http://www.matroska.org/

## 5.4. libMatroska

A C++ library to parse Matroska files

  * License: GNU Lesser General Public License v2.1 (see `doc/licenses/LGPL-2.1.txt`)
  * URL: http://www.matroska.org/

## 5.5. librmff

librmff is short for 'RealMedia file format access library'. It aims
at providing the programmer an easy way to read and write RealMedia
files.

  * License: GNU Lesser General Public License v2.1 (see `doc/licenses/LGPL-2.1.txt`)
  * URL: https://www.bunkus.org/videotools/librmff/index.html

## 5.6. nlohmann's JSON

JSON for Modern C++

  * License: MIT (see `doc/licenses/nlohmann-json-MIT.txt`)
  * URL: https://github.com/nlohmann/json

## 5.7. pugixml

An XML processing library

  * License: MIT (see `doc/licenses/pugixml-MIT.txt`)
  * URL: http://pugixml.org/

## 5.8. utf8-cpp

UTF-8 with C++ in a Portable Way

  * License: custom (see `doc/licenses/utf8-cpp-custom.txt`)
  * URL: http://utfcpp.sourceforge.net/

## 5.9. Oxygen icons and sound files

Most of the icons included in this package originate from the Oxygen
Project. These include all files in the `share/icons` sub-directory
safe for those whose name starts with `mkv`.

The preferred form of modification are the SVG icons. These are not
part of the binary distribution of MKVToolNix, but they are contained
in the source code in the `icons/scalable` sub-directory. You can
obtain the source code from the
[MKVToolNix website](https://mkvtoolnix.download/).

All of the sound files in the `share/sounds` sub-directory originate
from the Oxygen project.

  * License: GNU Lesser General Public License v3 (see `doc/licenses/LGPL-3.0.txt`)
  * URL: https://techbase.kde.org/Projects/Oxygen
