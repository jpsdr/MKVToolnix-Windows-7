#!/bin/bash

set -e

# Creates a tree with all the required libraries for use with the
# mingw cross compiler. The libraries are compiled appropriately.
# Read the file "Building.for.Windows.md" for instructions.

#
# SETUP -- adjust these variables if neccessary.
# You can also override them from the command line:
# INSTALL_DIR=/opt/mingw ARCHITECTURE=32 ./setup_cross_compilation_env.sh
#

# This defaults to a 64bit executable. If you need a 32bit executable
# then change ARCHITECTURE to 32. Alternatively set the HOST variable
# outside the script to something like "x86_64-w64-mingw32.static".
ARCHITECTURE=${ARCHITECTURE:-64}
# Installation defaults to ~/mxe.
INSTALL_DIR=${INSTALL_DIR:-$HOME/mxe}
# Leave PARALLEL empty if you want the script to use all of your CPU
# cores.
PARALLEL=${PARALLEL:-$(nproc --all)}

#
# END OF SETUP -- usually no need to change anything else
#

if [[ -z $HOST ]]; then
  if [[ "$ARCHITECTURE" == 32 ]]; then
    HOST=i686-w64-mingw32.static
  else
    HOST=x86_64-w64-mingw32.static
  fi
fi

SRCDIR=$(pwd)
LOGFILE=${LOGFILE:-$(mktemp -p '' mkvtoolnix_setup_cross_compilation_env.XXXXXX)}

function update_mingw_cross_env {
  if [[ ! -d $INSTALL_DIR ]]; then
    echo Retrieving the M cross environment build scripts >> $LOGFILE
    git clone --branch master https://codeberg.org/mbunkus/mxe $INSTALL_DIR >> $LOGFILE 2>&1
  else
    echo Updating the M cross environment build scripts >> $LOGFILE
    cd $INSTALL_DIR
    git fetch >> $LOGFILE 2>&1 \
      && git reset --hard origin/master >> $LOGFILE 2>&1 \
      && git config branch.$(git branch --show-current).merge refs/heads/master >> $LOGFILE 2>&1
  fi

  cd ${INSTALL_DIR}
  cat > settings.mk <<EOF
MXE_TARGETS = ${HOST}
MXE_USE_CCACHE =
MXE_PLUGIN_DIRS += plugins/gcc14
JOBS = ${PARALLEL}

MKVTOOLNIX_DEPENDENCIES=gettext libiconv zlib boost flac ogg pthreads vorbis cmark libdvdread gmp
MKVTOOLNIX_DEPENDENCIES+=qt6 qt6-qtmultimedia

LOCAL_PKG_LIST=\$(MKVTOOLNIX_DEPENDENCIES)
local-pkg-list: \$(LOCAL_PKG_LIST)
mkvtoolnix-deps: local-pkg-list
EOF
}

function create_run_configure_script {
  cd $SRCDIR

  echo Creating \'run_configure.sh\' script
  qtbin=${INSTALL_DIR}/usr/${HOST}/qt5/bin

  static_qt=""
  if [[ $HOST == *static* ]]; then
    static_qt="--enable-static-qt"
  fi

  cat > run_configure.sh <<EOF
#!/bin/bash

export PATH=${INSTALL_DIR}/usr/bin:$PATH
hash -r

./configure \\
  --host=${HOST} \\
  --with-boost="${INSTALL_DIR}/usr/${HOST}" \\
  ${static_qt} \\
  --with-moc=${qtbin}/moc --with-uic=${qtbin}/uic --with-rcc=${qtbin}/rcc \\
  --enable-mkvtoolnix \\
  "\$@"

exit \$?
EOF
  chmod 755 run_configure.sh

  echo Creating \'configure\'
  ./autogen.sh
  git submodule init
  git submodule update
}

function configure_mkvtoolnix {
  cd $SRCDIR

  echo Running configure.
  set +e
  ./run_configure.sh >> $LOGFILE 2>&1
  local result=$?
  set -e

  echo
  if [ $result -eq 0 ]; then
    echo 'Configuration went well. Congratulations. You can now run "rake"'
    echo 'after adding the mingw cross compiler installation directory to your PATH:'
    echo '  export PATH='${INSTALL_DIR}'/usr/bin:$PATH'
    echo '  hash -r'
    echo '  rake'
  else
    echo "Configuration failed. Look at ${LOGFILE} as well as"
    echo "at ./config.log for hints as to why."
  fi

  echo
  echo 'If you need to re-configure MKVToolNix then you can do that with'
  echo 'the script ./run_configure.sh. Any parameter you pass to run_configure.sh'
  echo 'will be passed to ./configure as well.'
}

function build_libraries {
  if [[ -n ${PKG_CACHE} ]]; then
    rmdir ${INSTALL_DIR}/pkg || true
    ln -fs ${PKG_CACHE} ${INSTALL_DIR}/pkg
  fi

  echo Building the cross-compiler and the required libraries
  cd ${INSTALL_DIR}
  make >> $LOGFILE 2>&1
}

# main

echo "Cross-compiling MKVToolNix. Log output can be found in ${LOGFILE}"
update_mingw_cross_env
build_libraries
create_run_configure_script
configure_mkvtoolnix
