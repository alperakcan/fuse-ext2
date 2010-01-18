/**
 * Copyright (c) 2008-2010 Alper Akcan <alper.akcan@gmail.com>
 * Copyright (c) 2009 Renzo Davoli <renzo@cs.unibo.it>
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

#include "fuse-ext2.h"

struct dir_walk_data {
	char *buf;
	fuse_fill_dir_t filler;
};

#define _USE_DIR_ITERATE2
#ifdef _USE_DIR_ITERATE2
static int walk_dir2 (ext2_ino_t dir, int   entry, struct ext2_dir_entry *dirent, int offset, int blocksize, char *buf, void *vpsid)
{
	if (dirent->name_len > 0) {
		int res;
		unsigned char type;
		int len;
		struct dir_walk_data *psid=(struct dir_walk_data *)vpsid;
		struct stat st;
		memset(&st, 0, sizeof(st));

		len=dirent->name_len & 0xff;
		dirent->name[len]=0; // bug wraparound

		switch  (dirent->name_len >> 8) {
			case EXT2_FT_UNKNOWN: type=DT_UNKNOWN;break;
			case EXT2_FT_REG_FILE:  type=DT_REG;break;
			case EXT2_FT_DIR: type=DT_DIR;break;
			case EXT2_FT_CHRDEV:  type=DT_CHR;break;
			case EXT2_FT_BLKDEV:  type=DT_BLK;break;
			case EXT2_FT_FIFO:  type=DT_FIFO;break;
			case EXT2_FT_SOCK:  type=DT_SOCK;break;
			case EXT2_FT_SYMLINK: type=DT_LNK;break;
			default:    type=DT_UNKNOWN;break;
		}
		st.st_ino=dirent->inode;
		st.st_mode=type<<12;
		debugf("%s %d %d %d",dirent->name,dirent->name_len &0xff, dirent->name_len >> 8,type);
		res = psid->filler(psid->buf, dirent->name, &st, 0);
	}
	return 0;
}
#else
static int walk_dir (struct ext2_dir_entry *de, int offset, int blocksize, char *buf, void *priv_data)
{
	int ret;
	size_t flen;
	char *fname;
	struct dir_walk_data *b = priv_data;

	debugf("enter");

	flen = de->name_len & 0xff;
	fname = (char *) malloc(sizeof(char) * (flen + 1));
	if (fname == NULL) {
		debugf("s = (char *) malloc(sizeof(char) * (%d + 1)); failed", flen);
		return -ENOMEM;
	}
	snprintf(fname, flen + 1, "%s", de->name);
	debugf("b->filler(b->buf, %s, NULL, 0);", fname);
	ret = b->filler(b->buf, fname, NULL, 0);
	free(fname);
	
	debugf("leave");
	return ret;
}
#endif

int op_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	int rt;
	errcode_t rc;
	ext2_ino_t ino;
	struct ext2_inode inode;
	struct dir_walk_data dwd={
		.buf = buf,
		.filler = filler};
	ext2_filsys e2fs = current_ext2fs();

	debugf("enter");
	debugf("path = %s", path);
	
	rt = do_readinode(e2fs, path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", path);
		return rt;
	}

#ifdef _USE_DIR_ITERATE2
	rc = ext2fs_dir_iterate2(e2fs,ino, DIRENT_FLAG_INCLUDE_EMPTY, NULL, walk_dir2, &dwd);
#else
	rc = ext2fs_dir_iterate(e2fs, ino, 0, NULL, walk_dir, &dwd);
#endif

	if (rc) {
		debugf("Error while trying to ext2fs_dir_iterate %s", path);
		return -EIO;
	}

	debugf("leave");
	return 0;
}
