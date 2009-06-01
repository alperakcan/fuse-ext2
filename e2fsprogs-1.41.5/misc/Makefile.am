
bin_PROGRAMS = \
	e2label \
	mke2fs

e2label_SOURCES = \
	e2label.c

mke2fs_SOURCES = \
	mke2fs.c \
	util.c \
	profile.c \
	prof_err.c \
	default_profile.c

mke2fs_CFLAGS = \
	-I$(top_srcdir)/e2fsprogs-1.41.5

mke2fs_LDFLAGS = \
	../ext2fs/libext2fs.a	\
	../et/libcom_err.a \
	../e2p/libe2p.a \
	../blkid/libblkid.a
