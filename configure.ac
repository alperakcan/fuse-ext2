
AC_INIT([fuse-ext2], [0.0.7], [alper.akcan@gmail.com])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_AR
AC_PROG_LIBTOOL
AC_PATH_PROG(CHMOD, chmod, :)

case $target_os in
     *linux*)	arch=linux;;
     *darwin*)	arch=darwin;;
     *)			arch=unknown;;
esac

# Large file support
AC_TYPE_SIZE_T
AC_TYPE_OFF_T
AC_DEFINE([_LARGE_FILE_SOURCE], [], [Large files support])
AC_DEFINE([_FILE_OFFSET_BITS], [64], [File Offset size])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_MAJOR
AC_CHECK_HEADERS([	\
	fcntl.h		\
	malloc.h	\
	mntent.h	\
	netinet/in.h	\
	paths.h		\
	stddef.h	\
	stdlib.h	\
	string.h	\
	linux/fd.h	\
	sys/file.h	\
	sys/ioctl.h	\
	sys/mount.h	\
	sys/param.h	\
	sys/statvfs.h	\
	sys/time.h	\
	sys/types.h	\
	sys/stat.h	\
	sys/mkdev.h	\
	sys/ioctl.h	\
	sys/syscall.h	\
	sys/resource.h	\
	sys/mman.h	\
	sys/prctl.h	\
	sys/disklabel.h \
	sys/queue.h	\
	sys/socket.h	\
	sys/un.h	\
	sys/sockio.h	\
	net/if.h	\
	netinet/in.h	\
	net/if_dl.h	\
	errno.h		\
	unistd.h	\
	utime.h		\
	getopt.h	\
	inttypes.h	\
])

AC_CHECK_HEADERS(sys/disk.h sys/mount.h,,,
[[
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
]])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_HEADER_TIME
AC_CHECK_DECL(llseek,[AC_DEFINE(HAVE_LLSEEK_PROTOTYPE, 1, [Define to 1 if you have the `llseek' prototype.])],,
		[#include <unistd.h>])
AC_CHECK_DECL(lseek64,[AC_DEFINE(HAVE_LSEEK64_PROTOTYPE, 1, [Define to 1 if you have the `lseek64' prototype.])],,
		[#define _LARGEFILE_SOURCE
		 #define _LARGEFILE64_SOURCE
		 #include <unistd.h>])

dnl Check to see if ssize_t was decleared
AC_CHECK_TYPE(ssize_t,[AC_DEFINE(HAVE_TYPE_SSIZE_T, 1, [Define to 1 if you have the `ssize_t' prototype.])],,
		[#include <sys/types.h>])

# Checks for library functions.
AC_FUNC_VPRINTF
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_GETMNTENT
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_UTIME_NULL
AC_CHECK_FUNCS([	\
	ftruncate		\
	getmntent		\
	getmntinfo		\
	getpagesize		\
	hasmntopt		\
	memmove			\
	memset			\
	munmap			\
	random			\
	select			\
	srand			\
	srandom			\
	strcasecmp      	\
	strchr			\
	strdup			\
	strerror		\
	strrchr			\
	strtol			\
	strtoul			\
	strtoull		\
	uname			\
	utime			\
	llseek			\
	lseek64			\
])

# Checks for libraries
AC_SEARCH_LIBS([fuse_main], [osxfuse fuse], [], [AC_MSG_ERROR([Can't find libfuse, please install it])])
AC_SEARCH_LIBS([com_err], [com_err], [], [AC_MSG_ERROR([Can't find e2progs, please install it])])
AC_SEARCH_LIBS([ext2fs_open], [ext2fs], [], [AC_MSG_ERROR([Can't find e2progs, please install it])])

AC_MSG_CHECKING([if FUSE on this system is too new for us])
AC_EGREP_CPP([fuse_version_yes], [
#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 27
#include "fuse.h"
#if FUSE_VERSION > 27
fuse_version_yes
#endif
], AC_DEFINE([FUSE_USE_VERSION], [27], [Version of FUSE interface]) AC_MSG_RESULT([yes]),
   AC_DEFINE([FUSE_USE_VERSION], [FUSE_VERSION], [Version of FUSE interface]) AC_MSG_RESULT([no]))

# Check extra parameters
AC_ARG_ENABLE([debug],
[  --enable-debug    	  enable noisy debug],
if test "$enableval" = "no"
then
	DEBUG_CMT=#
	echo "Disabling debug support"
else
	DEBUG_CMT=
	AC_DEFINE(ENABLE_DEBUG, 1, [Define to 1 if you want 'debug' support.])
	echo "Enabling debug support"
fi
,
DEBUG_CMT=
echo "Disabling debug support by default"
	AC_DEFINE(ENABLE_DEBUG, 0, [Define to 1 if you want 'debug' support.])
)

AC_ARG_WITH(pkgconfigdir,
            [  --with-pkgconfigdir=DIR pkgconfig file in DIR @<:@LIBDIR/pkgconfig@:>@],
            [pkgconfigdir=$withval],
            [pkgconfigdir='${libdir}/pkgconfig'])
AC_SUBST(pkgconfigdir)

AM_CONDITIONAL(LINUX, test "$arch" = linux)
AM_CONDITIONAL(DARWIN, test "$arch" = darwin)

AC_CONFIG_FILES([
	fuse-ext2.pc
	Makefile
	fuse-ext2/Makefile
	tools/Makefile
	tools/macosx/Makefile
	test/Makefile
	test/pjdfstest/Makefile
])

AC_OUTPUT
