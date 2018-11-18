#!/bin/bash

# stop script if an error is occurred
set -e

PATH=""

PATH=$(pwd)/gnu/bin:/usr/sbin:/usr/bin:/sbin:/bin:$PATH
export PATH
echo "$PATH"

PKG_CONFIG_PATH=$(pwd)/gnu/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
export PKG_CONFIG_PATH
echo "$PKG_CONFIG_PATH"

mkdir fuse-ext2.build
cd fuse-ext2.build

build_root=$(pwd)
echo "$build_root"

# if [ ! -d fuse-ext2 ]; then
#   git clone https://github.com/alperakcan/fuse-ext2.git
# fi

# m4
if [ ! -f m4-1.4.17.targ.gz ]; then
  curl -O -L http://ftp.gnu.org/gnu/m4/m4-1.4.17.tar.gz
fi
tar xvzf m4-1.4.17.tar.gz
cd m4-1.4.17
./configure --prefix="$build_root/gnu"
make
make install
cd ../

# autoconf
if [ ! -f autoconf-2.69.tar.gz ]; then
    curl -O -L http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
fi
tar zxvf autoconf-2.69.tar.gz 
cd autoconf-2.69
./configure --prefix="$build_root/gnu"
make
make install
cd ../

echo "====================================================="

echo "$PATH"


echo "====================================================="

# automake
if [ ! -f automake-1.15.tar.gz ]; then
    curl -O -L http://ftp.gnu.org/gnu/automake/automake-1.15.tar.gz
fi
tar zxvf automake-1.15.tar.gz 
cd automake-1.15

echo "====================================================="

pwd
echo "$PATH"

echo "====================================================="

PATH="$build_root/gnu/bin":$PATH

echo "====================================================="

pwd
echo "$PATH"

echo "====================================================="

# automake

./configure --prefix="$build_root/gnu"
make
make install
cd ../

echo "====================================================="

echo "$PATH"

echo "====================================================="

# libtool
if [ ! -f libtool-2.4.6.tar.gz ]; then
    curl -O -L http://ftpmirror.gnu.org/libtool/libtool-2.4.6.tar.gz
fi
tar zxvf libtool-2.4.6.tar.gz 
cd libtool-2.4.6
./configure --prefix="$build_root/gnu"
make
make install
cd ../

# e2fsprogs
if [ ! -f e2fsprogs-1.43.4.tar.gz ]; then
    curl -O -L https://www.kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v1.43.4/e2fsprogs-1.43.4.tar.gz
fi
tar zxvf e2fsprogs-1.43.4.tar.gz
cd e2fsprogs-1.43.4
./configure --prefix="$build_root/gnu" --disable-nls
make
make install
make install-libs
# sudo cp /opt/gnu/lib/pkgconfig/* /usr/local/lib/pkgconfig
cd ../

echo "$PATH"

echo "====================================================="

pwd

echo "====================================================="

cd ../..

echo "====================================================="

pwd

echo "====================================================="

./autogen.sh

CFLAGS="-idirafter$build_root/gnu/include -idirafter/usr/local/include/osxfuse/"
LDFLAGS="-L$build_root/gnu/lib -L/usr/local/lib"

export CFLAGS
export LDFLAGS

echo "====================================================="
echo "CFLAGS = '$CFLAGS'"
echo "LDFLAGS = '$LDFLAGS'"
echo "====================================================="

./configure
make

