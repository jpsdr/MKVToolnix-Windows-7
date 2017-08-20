export TARGET=$HOME/net/home/opt/mac
export SRCDIR=$HOME/net/home/prog/mac/source
export PACKAGE_DIR=$HOME/prog/mac/packages
export DOCBOOK_XSL_ROOT_DIR=$HOME/prog/mac/xsl-stylesheets
export CMPL=$HOME/tmp/compile
export PATH=${TARGET}/bin:$PATH
export CC="clang"
export CPP="clang -E"
export CXX="clang++"
export CXXCPP="clang++ -E"
export CFLAGS=""
export CXXFLAGS="-std=c++14 -fvisibility=hidden -fvisibility-inlines-hidden"
export QT_CXXFLAGS="-stdlib=libc++"
export MACOSX_DEPLOYMENT_TARGET="10.9"
export DRAKETHREADS=${DRAKETHREADS:-4}
export MAKEFLAGS="-j ${DRAKETHREADS}"
export SHARED_QT=1
export QTVER=${QTVER:-5.9.1}
export SIGNATURE_IDENTITY="Developer ID Application: Moritz Bunkus (YZ9DVS8D8C)"
