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

# m4
if [ ! -f m4-1.4.18.tar.xz ]; then
  curl -O -L https://ftp.gnu.org/gnu/m4/m4-1.4.18.tar.xz
fi
tar xvzf m4-1.4.18.tar.xz
cd m4-1.4.18
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
if [ ! -f automake-1.16.1.tar.xz ]; then
  curl -O -L https://ftp.gnu.org/gnu/automake/automake-1.16.1.tar.xz
fi
tar xzvf automake-1.16.1.tar.xz
cd automake-1.16.1

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
  curl -O -L https://ftp.wayne.edu/gnu/libtool/libtool-2.4.6.tar.xz
fi
tar xvzf libtool-2.4.6.tar.xz 
cd libtool-2.4.6
./configure --prefix="$build_root/gnu"
make
make install
cd ../

# e2fsprogs
if [ ! -f e2fsprogs-1.44.3.tar.xz ]; then
  curl -O -L https://mirrors.edge.kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v1.44.3/e2fsprogs-1.44.3.tar.xz
fi
tar xvzf e2fsprogs-1.44.3.tar.xz
cd e2fsprogs-1.44.3
./configure --prefix="$build_root/gnu" --disable-nls
make
make install
make install-libs
cp "$build_root"/gnu/lib/pkgconfig/* /usr/local/lib/pkgconfig
cd ../

# e2fsprogs | v1.44.4
# if [ ! -f e2fsprogs-1.44.4.tar.xz ]; then
#   curl -O -L https://mirrors.edge.kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v1.44.4/e2fsprogs-1.44.4.tar.xz
# fi
# tar xvzf e2fsprogs-1.44.4.tar.xz
# cd e2fsprogs-1.44.4
# ./configure --prefix="$build_root/gnu" --disable-nls
# make
# make install
# make install-libs
# cp "$build_root"/gnu/lib/pkgconfig/* /usr/local/lib/pkgconfig
# cd ../

echo "$PATH"

echo "====================================================="

pwd

echo "====================================================="

cd ../../

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

########
# uncomment the below line to install
###

# sudo make install
