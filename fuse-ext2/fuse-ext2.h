/**
 * Copyright (c) 2008-2009 Alper Akcan <alper.akcan@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the fuse-ext2
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef FUSEEXT2_H_
#define FUSEEXT2_H_

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <getopt.h>
#include <assert.h>
#include <errno.h>

#include <fuse.h>
#include <ext2fs/ext2fs.h>

#if !defined(FUSE_VERSION) || (FUSE_VERSION < 26)
#error "***********************************************************"
#error "*                                                         *"
#error "*     Compilation requires at least FUSE version 2.6.0!   *"
#error "*                                                         *"
#error "***********************************************************"
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define EXT2FS_FILE(efile) ((void *) (unsigned long) (efile))

struct options_s {
	int debug;
	int silent;
	int force;
	int readonly;
	char *mnt_point;
	char *options;
	char *device;
};

struct private_s {
	char *name;
	ext2_filsys fs;
};

extern struct options_s opts;
extern struct private_s priv;

#if ENABLE_DEBUG

static inline void debug_printf (const char *function, char *file, int line, const char *fmt, ...)
{
	va_list args;
	if (opts.debug == 0 || opts.silent == 1) {
		return;
	}
	printf("%s: ", PACKAGE);
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf(" [%s (%s:%d)]\n", function, file, line);
}

#define debugf(a...) { \
	debug_printf(__FUNCTION__, __FILE__, __LINE__, a); \
}

#else /* ENABLE_DEBUG */

#define debugf(a...) do { } while(0)

#endif /* ENABLE_DEBUG */

void * op_init (struct fuse_conn_info *conn);

void op_destroy (void *userdata);

/* helper functions */

int do_probe (void);

int do_check (const char *path);

void do_fillstatbuf (ext2_ino_t ino, struct ext2_inode *inode, struct stat *st);

int do_readinode (const char *path, ext2_ino_t *ino, struct ext2_inode *inode);

int do_killfilebyinode (ext2_ino_t ino, struct ext2_inode *inode);

/* read support */

int op_access (const char *path, int mask);

int op_fgetattr (const char *path, struct stat *stbuf, struct fuse_file_info *fi);

int op_getattr (const char *path, struct stat *stbuf);

ext2_file_t do_open (const char *path);

int op_open (const char *path, struct fuse_file_info *fi);

int op_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

int op_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

int op_readlink (const char *path, char *buf, size_t size);

int do_release (ext2_file_t efile);

int op_release (const char *path, struct fuse_file_info *fi);

int op_statfs(const char *path, struct statvfs *buf);

/* write support */

int do_modetoext2lag (mode_t mode);

int op_chmod (const char *path, mode_t mode);

int op_chown (const char *path, uid_t uid, gid_t gid);

int do_create (const char *path, mode_t mode);

int op_create (const char *path, mode_t mode, struct fuse_file_info *fi);

int op_flush (const char *path, struct fuse_file_info *fi);

int op_fsync (const char *path, int datasync, struct fuse_file_info *fi);

int op_mkdir (const char *path, mode_t mode);

int op_rmdir (const char *path);

int op_unlink (const char *path);

int op_utimens (const char *path, const struct timespec tv[2]);

size_t do_write (ext2_file_t efile, const char *buf, size_t size, off_t offset);

int op_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

int op_mknod (const char *path, mode_t mode, dev_t dev);

int op_symlink (const char *sourcename, const char *destname);

int op_truncate(const char *path, off_t length);

int op_link (const char *source, const char *dest);

#endif /* FUSEEXT2_H_ */
