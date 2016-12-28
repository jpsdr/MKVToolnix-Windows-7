export TARGET=$HOME/net/home/opt/mac
export SRCDIR=$HOME/net/home/prog/mac/source
export PACKAGE_DIR=$HOME/prog/mac/packages
export CMPL=$HOME/tmp/compile
export PATH=${TARGET}/bin:$PATH
export CC="clang"
export CPP="clang -E"
export CXX="clang++"
export CXXCPP="clang++ -E"
export CFLAGS="-mmacosx-version-min=10.9"
export CXXFLAGS="-std=c++14 -fvisibility=hidden -fvisibility-inlines-hidden -mmacosx-version-min=10.9"
export QT_CXXFLAGS="-stdlib=libc++ -mmacosx-version-min=10.9"
export MAKEFLAGS="-j 4"
export DRAKETHREADS=4
export SHARED_QT=1
export QTVER=${QTVER:-5.7.1}
