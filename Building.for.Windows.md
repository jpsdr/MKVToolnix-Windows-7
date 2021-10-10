Building MKVToolNix 62.0.0 for Windows
=====================================

There is currently only one supported way to build MKVToolNix for
Windows: on Linux using a MinGW cross compiler. It is known that you
can also build it on Windows itself with the MinGW gcc compiler, but
that's not supported officially as I don't have such a setup myself.

Earlier versions could still be built with Microsoft's Visual Studio /
Visual C++ programs, and those steps were described here as
well. However, current MKVToolNix versions require many features of
the C++11 and C++14 standards which Microsoft's compilers have had
spotty support for for a long time. Additionally the author doesn't
use Visual C++ himself and couldn't provide project files for it.

# 1. Building with a MinGW cross compiler

## 1.1. Preparations

You will need:

- a MinGW cross compiler
- roughly 4 GB of free space available

Luckily there's the [M cross environment project](http://mxe.cc/)
that provides an easy-to-use way of setting up the cross-compiler
and all required libraries.

MXE is a fast-changing project. In order to provide a stable basis for
compilation, the author maintains his own fork. That fork also includes
a couple of changes that cause libraries to be compiled only with the
features required by MKVToolNix saving compilation time and deployment
space. In order to retrieve that fork, you need `git`. Then do the
following:

    git clone https://gitlab.com/mbunkus/mxe $HOME/mxe

The rest of this guide assumes that you've unpacked MXE
into the directory `$HOME/mxe`.

## 1.2. Automatic build script

MKVToolNix contains a script that can download, compile and install
all required libraries into the directory `$HOME/mxe`.

If the script does not work or you want to do everything yourself,
you'll find instructions for manual compilation in section 1.3.

### 1.2.1. Script configuration

The script is called `packaging/windows/setup_cross_compilation_env.sh`.
It contains the following variables that can be adjusted to fit your
needs:

    ARCHITECTURE=64

The architecture (64-bit vs 32-bit) that the binaries will be built
for. The majority of users today run a 64-bit Windows, therefore 64 is
the default. If you run a 32-bit version of Windows, change this to 32.

    INSTALL_DIR=$HOME/mxe

Base installation directory

    PARALLEL=

Number of processes to execute in parallel. Will be set to the number
of cores available if left empty.

### 1.2.2. Execution

From the MKVToolNix source directory run:

    ./packaging/windows/setup_cross_compilation_env.sh

If everything works fine, you'll end up with a configured MKVToolNix
source tree. You just have to run `rake` afterwards.

## 1.3. Manual installation

First you will need the MXE build scripts. Get them by
downloading them (see section 1.1. above) and unpacking them into
`$HOME/mxe`.

Next, build the required libraries (change `MXE_TARGETS` to
`i686-w64-mingw32.static` if you need a 32-bit build instead of a 64-bit
one, and increase `JOBS` if you have more than one core):

    cd $HOME/mxe
    make MXE_TARGETS=x86_64-w64-mingw32.static MXE_PLUGIN_DIRS=plugins/gcc9 \
      JOBS=2 \
      gettext libiconv zlib boost file flac lzo ogg pthreads vorbis cmark \
      libdvdread qtbase qttranslations qtwinextras

Append the installation directory to your `PATH` variable:

    export PATH=$PATH:$HOME/mxe/usr/bin
    hash -r

Finally, configure MKVToolNix (the `host=…` spec must match the
`MXE_TARGETS` spec from above):

    cd $HOME/path/to/mkvtoolnix-source
    host=x86_64-w64-mingw32.static
    qtbin=$HOME/mxe/usr/${host}/qt5/bin
    ./configure \
      --host=${host} \
      --enable-static-qt \
      --with-moc=${qtbin}/moc --with-uic=${qtbin}/uic --with-rcc=${qtbin}/rcc \
      --with-boost=$HOME/mxe/usr/${host}

If everything works, build it:

    rake

You're done.
