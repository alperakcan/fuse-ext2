# Fuse Ext2

> Fuse-ext2 is a EXT2/EXT3/EXT4 Filesystem support for _FUSE_.

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

**Note for Mac OS X 10.11.x "El Capitan" and later**: in order to successfully run the command `sudo make install`,
you have to disable [System Integrity Protection](https://support.apple.com/en-us/HT204899) (SIP) by following these
instructions (from [Lifehacker](http://lifehacker.com/how-to-fix-os-x-el-capitans-annoyances-1733836821)):

> 1. Reboot your Mac into Recovery Mode by restarting your computer and holding down _Command+R_ until the Apple logo appears on your screen.
> 2. Click _Utilities > Terminal_.
> 3. In the Terminal window, type in `csrutil disable` and press Enter.
> 4. Restart your Mac.
> 
>SIP is now disabled. You can reenable it at any by following the above instructions and substituting `csrutil enable` in for the Terminal command.

Dependecies:

[OSXfuse](https://osxfuse.github.io) io no need to install with MacFuse compatibility.

The easiest way is using [Homebrew](http://brew.sh/):

```shell
$ brew install e2fsprogs m4 automake autoconf libtool
$ git clone https://github.com/alperakcan/fuse-ext2.git
$ cd fuse-ext2
$ ./autogen.sh
$ PKG_CONFIG_PATH="$(brew --prefix e2fsprogs)/lib/pkgconfig" CFLAGS="-idirafter/$(brew --prefix e2fsprogs)/include -idirafter/usr/local/include/osxfuse" LDFLAGS="-L$(brew --prefix e2fsprogs)/lib" ./configure
$ make 
$ sudo make install
```

Build:
	
Build **from source** depends on:

* m4
* autoconf
* automake
* libtool
* e2fsprogs

```shell
export PATH=/opt/gnu/bin:$PATH

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
curl -O https://www.kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v1.42.12/e2fsprogs-1.42.12.tar.gz
tar -zxvf e2fsprogs-1.42.12.tar.gz
cd e2fsprogs-1.42.12
./configure --prefix=/opt/gnu
make
sudo make install
sudo make install-libs
cd ../
	
# fuse-ext2
export PATH=/opt/gnu/bin:$PATH
./autogen.sh
CFLAGS="-idirafter/opt/gnu/include -idirafter/usr/local/include/osxfuse/" LDFLAGS="-L/opt/gnu/lib -L/usr/local/lib" ./configure
make
sudo make install
```

# Test

```shell
dd if=/dev/zero of=test/fs.ext2 bs=1024 count=102400
mkfs.ext4 test/fs.ext2
fuse-ext2 test/fs.ext2 /mnt/fs.ext2 -o debug,rw+
```

# Usage

See [Man page](http://man.cx/fuseext2(1)) for options.

```
Usage:    fuse-ext2 <device|image_file> <mount_point> [-o option[,...]]

Options:  ro, force, allow_others
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