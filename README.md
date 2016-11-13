# Fuse Ext2

> Fuse-ext2 is a EXT2/EXT3/EXT4 Filesystem support for _FUSE_.
this fork is a build tested under XCode 8.1, OS X 10.11.
changed to a version of e2fsprogs that builds cleanly following these instructions, and removed offending line from configure.ac that breaks original build.
write support has been tested and seems to work. using this i'm able to read and write ext3 nemesis (http://mrn.sixbit.org) volumes on OS X, providing a common filesystem for the OS X, Linux and FreeBSD versions.
i'll try to provide a binary package for 10.11, but this build works now as -is. 

## Dependencies

Fuse-ext2 requires at least Fuse version 2.6.0 for Linux.
For Mac OS X latest version of OSXFuse 2.7.5.

1. Linux: [Fuse](http://fuse.sourceforge.net/)
1. Mac OS: [OSXFuse](https://osxfuse.github.io)


## Build

### Linux:

Dependencies:

```shell
# fuse
$ apt-get install libfuse
```

Build from source depends on:

* m4
* autoconf
* automake
* libtool
* fuse-dev
* e2fsprogs-dev

```shell
$ apt-get install m4 autoconf automake libtool
$ apt-get install libfuse-dev e2fslibs-dev
	
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
```

You can use `checkinstall` or some other equivalent tool to generate install 
package for your distribution.

### Mac OS:

Dependencies:

[OSXfuse](https://osxfuse.github.io)
Build **from source** depends on:

* m4
* autoconf
* automake
* libtool
* e2fsprogs

```shell
export PATH=/opt/gnu/bin:$PATH
export PKG_CONFIG_PATH=/opt/gnu/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

mkdir gnu
cd gnu
	
# m4
curl -O http://ftp.gnu.org/gnu/m4/m4-1.4.17.tar.gz
tar -zxvf m4-1.4.17.tar.gz 
cd m4-1.4.17
./configure --prefix=/opt/gnu
make -j 16
sudo make install
cd ../
	
# autoconf
curl -O http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
tar -zxvf autoconf-2.69.tar.gz 
cd autoconf-2.69
./configure --prefix=/opt/gnu
make
sudo make install
cd ../
	
# automake
curl -O http://ftp.gnu.org/gnu/automake/automake-1.15.tar.gz
tar -zxvf automake-1.15.tar.gz 
cd automake-1.15
./configure --prefix=/opt/gnu
make
sudo make install
cd ../
	
# libtool
curl -LO http://ftpmirror.gnu.org/libtool/libtool-2.4.6.tar.gz
tar -zxvf libtool-2.4.6.tar.gz 
cd libtool-2.4.6
./configure --prefix=/opt/gnu
make
sudo make install
cd ../

# e2fsprogs
curl -O https://www.kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v1.42.12/e2fsprogs-1.43.3.tar.gz
tar -zxvf e2fsprogs-1.43.3.tar.gz
cd e2fsprogs-1.43.3
./configure --prefix=/opt/gnu --disable-nls
make
sudo make install
sudo make install-libs
sudo cp /opt/gnu/lib/pkgconfig/* /usr/local/lib/pkgconfig
cd ../
	
# fuse-ext2
export PATH=/opt/gnu/bin:$PATH
export PKG_CONFIG_PATH=/opt/gnu/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

./autogen.sh
CFLAGS="-idirafter/opt/gnu/include -idirafter/usr/local/include/osxfuse/" LDFLAGS="-L/opt/gnu/lib -L/usr/local/lib" ./configure
make
sudo make install
```

# Test
the e2fsprogs live in /opt/gnu/bin and /opt/gnu/sbin, fuse-ext2 is in /usr/local/bin

```shell
dd if=/dev/zero of=test/fs.ext2 bs=1024 count=102400
/opt/gnu/sbin/mkfs.ext4 test/fs.ext2
fuse-ext2 test/fs.ext2 /mnt/fs.ext2 -o debug,rw+
```

# Usage

See [Man page](http://man.cx/fuseext2(1)) for options.

```
Usage:    fuse-ext2 <device|image_file> <mount_point> [-o option[,...]]

Options:  ro, rw+, force, allow_others
          Please see details in the manual.

Example:  fuse-ext2 /dev/sda1 /mnt/sda1
```

# Bugs

* Multithread support is broken for now, so forcing fuse to work single thread.
* there are no known bugs for read-only mode, read only mode should be ok for every one.
* although, write support is available please do not mount your filesystems with write support unless you do not have anything to loose.

Please send output the output of below command while reporting bugs as [GitHub Issue](https://github.com/alperakcan/fuse-ext2/issues/new).
Before submitting a bug report, please look at the [existing issues](https://github.com/alperakcan/fuse-ext2/issues?utf8=%E2%9C%93&q=is%3Aissue) first.

```shell
$ /usr/local/bin/fuse-ext2 -v /dev/path /mnt/point -o debug
```

# Important: Partition Labels

Please **do not** use comma `,` in partition labels.

**Wrong:** `e2label /dev/disk0s3 "linux,ext3"`

**Correct:** `e2label /dev/disk0s3 "linux-ext3"`

# Contact

Alper Akcan <alper.akcan@gmail.com>
