/**
 * Copyright (c) 2008-2010 Alper Akcan <alper.akcan@gmail.com>
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
#include <dirent.h>
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

/* extra definitions not yet included in ext2fs.h */
#define EXT2_FILE_SHARED_INODE 0x8000
errcode_t ext2fs_file_close2(ext2_file_t file, void (*close_callback) (struct ext2_inode *inode, int flags));

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define EXT2FS_FILE(efile) ((void *) (unsigned long) (efile))
/* max timeout to flush bitmaps, to reduce inconsistencies */
#define FLUSH_BITMAPS_TIMEOUT 10

struct extfs_data {
	unsigned char debug;
	unsigned char silent;
	unsigned char force;
	unsigned char readonly;
	time_t last_flush;
	char *mnt_point;
	char *options;
	char *device;
	char *volname;
	ext2_filsys e2fs;
};

static inline ext2_filsys current_ext2fs(void)
{
	struct fuse_context *mycontext=fuse_get_context();
	struct extfs_data *e2data=mycontext->private_data;
	time_t now=time(NULL);
	if ((now - e2data->last_flush) > FLUSH_BITMAPS_TIMEOUT) {
		ext2fs_write_bitmaps(e2data->e2fs);
		e2data->last_flush=now;
	}
	return (ext2_filsys) e2data->e2fs;
}

#if ENABLE_DEBUG

static inline void debug_printf (const char *function, char *file, int line, const char *fmt, ...)
{
	va_list args;
	struct fuse_context *mycontext=fuse_get_context();
	struct extfs_data *e2data=mycontext->private_data;
	if (e2data && (e2data->debug == 0 || e2data->silent == 1)) {
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

static inline void debug_main_printf (const char *function, char *file, int line, const char *fmt, ...)
{
	va_list args;
	printf("%s: ", PACKAGE);
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf(" [%s (%s:%d)]\n", function, file, line);
}

#define debugf_main(a...) { \
	debug_main_printf(__FUNCTION__, __FILE__, __LINE__, a); \
}

#else /* ENABLE_DEBUG */

#define debugf(a...) do { } while(0)
#define debugf_main(a...) do { } while(0)

#endif /* ENABLE_DEBUG */

struct ext2_vnode;

struct ext2_vnode * vnode_get (ext2_filsys e2fs, ext2_ino_t ino);

int vnode_put (struct ext2_vnode *vnode, int dirty);

static inline struct ext2_inode * vnode2inode (struct ext2_vnode *vnode)
{
	return (struct ext2_inode *) vnode;
}

void * op_init (struct fuse_conn_info *conn);

void op_destroy (void *userdata);

/* helper functions */

int do_probe (struct extfs_data *opts);

int do_label (void);

int do_check (const char *path);

int do_check_split(const char *path, char **dirname,char **basename);

void free_split(char *dirname, char *basename);

void do_fillstatbuf (ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode *inode, struct stat *st);

int do_readinode (ext2_filsys e2fs, const char *path, ext2_ino_t *ino, struct ext2_inode *inode);

int do_readvnode (ext2_filsys e2fs, const char *path, ext2_ino_t *ino, struct ext2_vnode **vnode);

int do_killfilebyinode (ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode *inode);

/* read support */

int op_access (const char *path, int mask);

int op_fgetattr (const char *path, struct stat *stbuf, struct fuse_file_info *fi);

int op_getattr (const char *path, struct stat *stbuf);

ext2_file_t do_open (ext2_filsys e2fs, const char *path, int flags);

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

int do_create (ext2_filsys e2fs, const char *path, mode_t mode, dev_t dev, const char *fastsymlink);

int op_create (const char *path, mode_t mode, struct fuse_file_info *fi);

int op_flush (const char *path, struct fuse_file_info *fi);

int op_fsync (const char *path, int datasync, struct fuse_file_info *fi);

int op_mkdir (const char *path, mode_t mode);

int do_check_empty_dir(ext2_filsys e2fs, ext2_ino_t ino);

int op_rmdir (const char *path);

int op_unlink (const char *path);

int op_utimens (const char *path, const struct timespec tv[2]);

size_t do_write (ext2_file_t efile, const char *buf, size_t size, off_t offset);

int op_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

int op_mknod (const char *path, mode_t mode, dev_t dev);

int op_symlink (const char *sourcename, const char *destname);

int op_truncate(const char *path, off_t length);

int op_ftruncate(const char *path, off_t length, struct fuse_file_info *fi);

int op_link (const char *source, const char *dest);

int op_rename (const char *source, const char *dest);

#endif /* FUSEEXT2_H_ */
