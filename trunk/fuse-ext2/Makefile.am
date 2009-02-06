
bin_PROGRAMS =		\
	fuse-ext2	\
	fuse-ext2.probe

fuse_ext2_probe_SOURCES =	\
	fuse-ext2.h \
	fuse-ext2.probe.c \
	do_probe.c

fuse_ext2_probe_CFLAGS =			\
	-Wall					\
	-DHAVE_CONFIG_H				\
	-D_FILE_OFFSET_BITS=64			\
	-I$(top_srcdir)/e2fsprogs-1.41.1

if DARWIN
fuse_ext2_probe_CFLAGS +=			\
	-D__FreeBSD__=10
endif

fuse_ext2_probe_LDADD =				\
	../e2fsprogs-1.41.1/ext2fs/libext2fs.a	\
	../e2fsprogs-1.41.1/et/libcom_err.a	\
	-lfuse

fuse_ext2_SOURCES =	\
	fuse-ext2.h \
	fuse-ext2.c \
	do_probe.c \
	do_check.c \
	do_fillstatbuf.c \
	do_readinode.c \
	do_killfilebyinode.c \
	op_init.c \
	op_destroy.c \
	op_access.c \
	op_fgetattr.c \
	op_getattr.c \
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
	op_truncate.c

fuse_ext2_CFLAGS =				\
	-Wall					\
	-DHAVE_CONFIG_H				\
	-D_FILE_OFFSET_BITS=64			\
	-I$(top_srcdir)/e2fsprogs-1.41.1

if DARWIN
fuse_ext2_CFLAGS +=				\
	-D__FreeBSD__=10
endif

fuse_ext2_LDADD =				\
	../e2fsprogs-1.41.1/ext2fs/libext2fs.a	\
	../e2fsprogs-1.41.1/et/libcom_err.a	\
	-lfuse

if DARWIN
install-exec-local:
	$(INSTALL) -d "$(DESTDIR)/usr/sbin"
	$(LN_S) -f "/System/Library/Filesystems/fuse-ext2.fs/mount_fuse-ext2" "$(DESTDIR)/sbin/mount_fuse-ext2"
endif
if LINUX
install-exec-local:
	$(INSTALL) -d "$(DESTDIR)/usr/sbin"
	$(LN_S) -f "$(DESTDIR)/$(bindir)/fuse-ext2" "$(DESTDIR)/sbin/mount.fuse-ext2"
endif
