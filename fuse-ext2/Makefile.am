
DEVELFLAGS =

bin_PROGRAMS = \
	fuse-ext2 \
	fuse-ext2.probe

if LINUX
moddir = $(libdir)/umview/modules
mod_LTLIBRARIES = umfuseext2.la
endif

fuse_ext2_probe_SOURCES = \
	fuse-ext2.h \
	fuse-ext2.probe.c \
	do_probe.c

fuse_ext2_probe_CFLAGS = \
	-Wall \
	-DHAVE_CONFIG_H \
	-I/usr/local/include

fuse_ext2_SOURCES =	\
	fuse-ext2.h \
	fuse-ext2.c \
	do_probe.c \
	do_check.c \
	do_fillstatbuf.c \
	do_readinode.c \
	do_writeinode.c \
	do_killfilebyinode.c \
	op_init.c \
	op_destroy.c \
	op_access.c \
	op_fgetattr.c \
	op_getattr.c \
	op_getxattr.c \
	op_open.c \
	op_read.c \
	op_readdir.c \
	op_readlink.c \
	op_release.c \
	op_statfs.c \
	op_chown.c \
	op_chmod.c \
	op_create.c \
	op_flush.c \
	op_fsync.c \
	op_mkdir.c \
	op_rmdir.c \
	op_unlink.c \
	op_utimens.c \
	op_write.c \
	op_mknod.c \
	op_symlink.c \
	op_truncate.c \
	op_link.c \
	op_rename.c

umfuseext2_la_SOURCES = \
	fuse-ext2.h \
	fuse-ext2.c \
	do_probe.c \
	do_check.c \
	do_fillstatbuf.c \
	do_readinode.c \
	do_writeinode.c \
	do_killfilebyinode.c \
	op_init.c \
	op_destroy.c \
	op_access.c \
	op_fgetattr.c \
	op_getattr.c \
	op_getxattr.c \
	op_open.c \
	op_read.c \
	op_readdir.c \
	op_readlink.c \
	op_release.c \
	op_statfs.c \
	op_chown.c \
	op_chmod.c \
	op_create.c \
	op_flush.c \
	op_fsync.c \
	op_mkdir.c \
	op_rmdir.c \
	op_unlink.c \
	op_utimens.c \
	op_write.c \
	op_mknod.c \
	op_symlink.c \
	op_truncate.c \
	op_link.c \
	op_rename.c

umfuseext2_la_CFLAGS = \
	-Wall \
	-DHAVE_CONFIG_H \
	-D_GNU_SOURCE \
	$(DEVELFLAGS) \
	-I$(includedir)/umview \
	-I/usr/local/include

umfuseext2_la_LDFLAGS = \
	-module \
	-avoid-version \
	-export-dynamic \
	-lext2fs

fuse_ext2_CFLAGS = \
	-Wall \
	-DHAVE_CONFIG_H \
	-I/usr/local/include

if DARWIN
bin_PROGRAMS += \
	fuse-ext2.wait \
	fuse-ext2.install \
	fuse-ext2.uninstall

fuse_ext2_wait_SOURCES = \
	fuse-ext2.wait.m

fuse_ext2_wait_CFLAGS = \
	-Wall \
	-D__FreeBSD__=10

fuse_ext2_wait_LDFLAGS = \
	-framework CoreFoundation

fuse_ext2_install_SOURCES = \
	fuse-ext2.install.m

fuse_ext2_install_CFLAGS = \
	-ObjC

fuse_ext2_install_LDFLAGS = \
	-framework Foundation

fuse_ext2_uninstall_SOURCES = \
	fuse-ext2.uninstall.m

fuse_ext2_probe_CFLAGS += \
	-D__FreeBSD__=10

fuse_ext2_CFLAGS +=	\
	-D__FreeBSD__=10
endif

if DARWIN
install-exec-local:
	$(INSTALL) -d "$(DESTDIR)/$(sbindir)"
	$(LN_S) -f "/Library/Filesystems/fuse-ext2.fs/Contents/Resources/mount_fuse-ext2" "$(DESTDIR)/$(sbindir)/mount_fuse-ext2"
	$(LN_S) -f "/usr/local/opt/e2fsprogs/sbin/e2label" "$(DESTDIR)/$(sbindir)/e2label"
	$(LN_S) -f "/usr/local/opt/e2fsprogs/sbin/mke2fs" "$(DESTDIR)/$(sbindir)/mke2fs"
endif
if LINUX
install-data-hook:
	cd "$(DESTDIR)/$(moddir)" && rm -f $(mod_LTLIBRARIES)

install-exec-local:
	$(INSTALL) -d "$(DESTDIR)/$(sbindir)"
	$(LN_S) -f "$(bindir)/fuse-ext2" "$(DESTDIR)/$(sbindir)/mount.fuse-ext2"
endif
