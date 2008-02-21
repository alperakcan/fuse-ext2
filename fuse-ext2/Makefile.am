
bin_PROGRAMS =		\
	fuse-ext2

fuse_ext2_SOURCES =	\
	fuse-ext2.h \
	fuse-ext2.c \
	do_fillstatbuf.c \
	do_readinode.c \
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
	op_flush.c \
	op_fsync.c \
	op_mkdir.c \
	op_rmdir.c

fuse_ext2_CFLAGS =						\
	-Wall								\
	-DHAVE_CONFIG_H						\
	-D_FILE_OFFSET_BITS=64				\
	-I$(top_srcdir)/e2fsprogs-1.40.6

if DARWIN
fuse_ext2_CFLAGS +=						\
	-D__FreeBSD__=10
endif

fuse_ext2_LDADD =							\
	../e2fsprogs-1.40.6/ext2fs/libext2fs.a	\
	../e2fsprogs-1.40.6/et/libcom_err.a		\
	-lfuse

if DARWIN
install-exec-local:
	$(INSTALL) -d "$(DESTDIR)/usr/sbin"
	$(LN_S) -f "$(DESTDIR)/$(bindir)/fuse-ext2" "$(DESTDIR)/usr/sbin/mount_ext2"
endif
if LINUX
install-exec-local:
	$(INSTALL) -d "$(DESTDIR)/usr/sbin"
	$(LN_S) -f "$(DESTDIR)/$(bindir)/fuse-ext2" "$(DESTDIR)/usr/sbin/mount.fuse-ext2"
endif
