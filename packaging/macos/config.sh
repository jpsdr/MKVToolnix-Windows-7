export TARGET=$HOME/opt
export SRCDIR=$HOME/opt/source
export PACKAGE_DIR=$HOME/opt/packages
export DOCBOOK_XSL_ROOT_DIR=$HOME/opt/xsl-stylesheets
export CMPL=$HOME/tmp/compile
export PATH=${TARGET}/bin:$PATH
export CC="clang"
export CPP="clang -E"
export CXX="clang++"
export CXXCPP="clang++ -E"
export CFLAGS=""
export CXXFLAGS="-std=c++17 -Wno-unused-parameter -Wno-deprecated-declarations"
export QT_CXXFLAGS="-stdlib=libc++"
export MACOSX_DEPLOYMENT_TARGET="13"
export DRAKETHREADS=${DRAKETHREADS:-4}
export MAKEFLAGS="-j ${DRAKETHREADS}"
export QTVER=${QTVER:-6.5.3}
export SIGNATURE_IDENTITY="Developer ID Application: Moritz Bunkus (YZ9DVS8D8C)"
export DYLD_LIBRARY_PATH=${TARGET}/lib:${DYLD_LIBRARY_PATH}
