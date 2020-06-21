# Fuse Ext2

**Fuse-ext2** is an EXT2/EXT3/EXT4 filesystem for  [**FUSE**](https://github.com/osxfuse/fuse), and is built to work with [**osxfuse**](https://github.com/osxfuse/osxfuse).

## Dependencies

**Fuse-ext2** requires at least Fuse version 2.6.0 for Linux.  
**Fuse-ext2** requires at least **Fuse for macOS** version 2.7.5 or greater.

- Linux: [Fuse](http://fuse.sourceforge.net/)
- macOS: [Fuse for macOS](https://osxfuse.github.io)

### Alternate Install method of _Fuse for macOS_

**Fuse for macOS** can be installed via [homebrew](http://brew.sh) if [Homebrew-Cask](https://caskroom.github.io/) has been tapped.

To tap **Homebrew-Cask**:

```bash
brew tap homebrew/cask
```

To verify that the the above tap is a part of `brew`:

```bash
brew tap
```

Look for **`homebrew/cask`** in the output.

To install **Fuse for macOS** using brew:

```bash
brew cask install osxfuse
```

## Building

### Debian/Ubuntu:

Building from source depends on the following:

* m4
* autoconf
* automake
* libtool
* libfuse-dev
* e2fsprogs
* comerr-dev
* e2fslibs-dev

```shell
$ sudo apt-get install m4 autoconf automake libtool
$ sudo apt-get install libfuse-dev e2fsprogs comerr-dev e2fslibs-dev
	
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
```

### Fedora 
```bash 
$ sudo dnf install @development-tools m4 autoconf automake libtool e2fsprogs libcom_err-devel fuse-libs e2fsprogs-devel
# build part 
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
```

You can use `checkinstall` or some other equivalent tool to generate an install 
package for your distribution.

### FreeBSD:

Install via pkg:

```shell
$ pkg install sysutils/fusefs-ext2
```

Building via ports:

```shell
$ cd /usr/ports/sysutils/fusefs-ext2
$ make install clean
```

### macOS:

Dependencies:

[OSXfuse](https://osxfuse.github.io)

Building **from source** depends on the following:

* m4
* autoconf
* automake
* libtool
* e2fsprogs
* xcode-select

Copy and paste this into a file such as `/tmp/ext4/script.sh`, but do *not* name the file `install.sh`. Remember to `chmod +x script.sh`. Run it 
from that directory - `./script.sh`

```shell
#!/bin/sh
export PATH=/opt/gnu/bin:$PATH
export PKG_CONFIG_PATH=/opt/gnu/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

mkdir fuse-ext2.build
cd fuse-ext2.build

if [ ! -d fuse-ext2 ]; then
    git clone https://github.com/alperakcan/fuse-ext2.git	
fi

# m4
if [ ! -f m4-1.4.17.tar.gz ]; then
    curl -O -L http://ftp.gnu.org/gnu/m4/m4-1.4.17.tar.gz
fi
tar -zxvf m4-1.4.17.tar.gz 
cd m4-1.4.17
./configure --prefix=/opt/gnu
make -j 16
sudo make install
cd ../
    
# autoconf
if [ ! -f autoconf-2.69.tar.gz ]; then
    curl -O -L http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
fi
tar -zxvf autoconf-2.69.tar.gz 
cd autoconf-2.69
./configure --prefix=/opt/gnu
make
sudo make install
cd ../
    
# automake
if [ ! -f automake-1.15.tar.gz ]; then
    curl -O -L http://ftp.gnu.org/gnu/automake/automake-1.15.tar.gz
fi
tar -zxvf automake-1.15.tar.gz 
cd automake-1.15
./configure --prefix=/opt/gnu
make
sudo make install
cd ../
    
# libtool
if [ ! -f libtool-2.4.6.tar.gz ]; then
    curl -O -L http://ftpmirror.gnu.org/libtool/libtool-2.4.6.tar.gz
fi
tar -zxvf libtool-2.4.6.tar.gz 
cd libtool-2.4.6
./configure --prefix=/opt/gnu
make
sudo make install
cd ../

# e2fsprogs
if [ ! -f e2fsprogs-1.43.4.tar.gz ]; then
    curl -O -L https://www.kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v1.43.4/e2fsprogs-1.43.4.tar.gz
fi
tar -zxvf e2fsprogs-1.43.4.tar.gz
cd e2fsprogs-1.43.4
./configure --prefix=/opt/gnu --disable-nls
make
sudo make install
sudo make install-libs
sudo cp /opt/gnu/lib/pkgconfig/* /usr/local/lib/pkgconfig
cd ../
    
# fuse-ext2
export PATH=/opt/gnu/bin:$PATH
export PKG_CONFIG_PATH=/opt/gnu/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

cd fuse-ext2
./autogen.sh
CFLAGS="-idirafter/opt/gnu/include -idirafter/usr/local/include/osxfuse/" LDFLAGS="-L/opt/gnu/lib -L/usr/local/lib" ./configure
make
sudo make install
```

# Test

The e2fsprogs live in /opt/gnu/bin and /opt/gnu/sbin. fuse-ext2 is in /usr/local/bin.

```shell
cd
dd if=/dev/zero of=/tmp/test-fs.ext4 bs=1024 count=102400  
/opt/gnu/sbin/mkfs.ext4 /tmp/test-fs.ext4
mkdir -p ~/mnt/fuse-ext2.test-fs.ext4
fuse-ext2 /tmp/fuse-ext2.test-fs.ext4 ~/mnt/fuse-ext2.test-fs.ext4 -o rw+,allow_other,uid=501,gid=20
```

To verify the **UID** and **GID** of the user mounting the file system:

```shell
id
```

To verify the file system has mounted properly:

```shell
mount
```

# Usage

See the [Man page](http://man.cx/fuseext2(1)) for options.

```
Usage:    fuse-ext2 <device|image_file> <mount_point> [-o option[,...]]

Options:  ro, rw+, force, allow_other
          Please see details in the manual.

Example:  fuse-ext2 /dev/sda1 /mnt/sda1
```

# Bugs

* Multithread support is broken for now, so fuse operates in a single thread.
* There are no known bugs for read-only mode, read only mode should be ok for everyone.
* Even though write support is available, _please do not mount your filesystems with write support unless you have nothing to lose._

Please send the output of the command below when reporting bugs as a [GitHub Issue](https://github.com/alperakcan/fuse-ext2/issues/new).
Before submitting a bug report, please look at the [existing issues](https://github.com/alperakcan/fuse-ext2/issues?utf8=%E2%9C%93&q=is%3Aissue) first.

```shell
$ /usr/local/bin/fuse-ext2 -v /dev/path /mnt/point -o debug
```

# Important: Partition Labels

Please **do not** use commas `,` in partition labels.

**Wrong:** `e2label /dev/disk0s3 "linux,ext3"`

**Correct:** `e2label /dev/disk0s3 "linux-ext3"`

# Contact

Alper Akcan <alper.akcan@gmail.com>
