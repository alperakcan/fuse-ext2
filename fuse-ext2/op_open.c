/**
 * Copyright (c) 2008-2015 Alper Akcan <alper.akcan@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "fuse-ext2.h"

ext2_file_t do_open (ext2_filsys e2fs, const char *path, int flags)
{
	int rt;
	errcode_t rc;
	ext2_ino_t ino;
	ext2_file_t efile;
	struct ext2_inode inode;
	struct fuse_context *cntx = fuse_get_context();
	struct extfs_data *e2data = cntx->private_data;

	debugf("enter");
	debugf("path = %s", path);

	rt = do_check(path);
	if (rt != 0) {
		debugf("do_check(%s); failed", path);
		return NULL;
	}

	rt = do_readinode(e2fs, path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", path);
		return NULL;
	}

	rc = ext2fs_file_open2(
			e2fs,
			ino,
			&inode,
			(((flags & O_ACCMODE) != 0) ? EXT2_FILE_WRITE : 0) | EXT2_FILE_SHARED_INODE, 
			&efile);
	if (rc) {
		return NULL;
	}

	if (e2data->readonly == 0) {
		inode.i_atime = e2fs->now ? e2fs->now : time(NULL);
		rt = do_writeinode(e2fs, ino, &inode);
		if (rt) {
			debugf("do_writeinode(%s, &ino, &inode); failed", path);
			return NULL;
		}
	}

	debugf("leave");
	return efile;
}

int op_open (const char *path, struct fuse_file_info *fi)
{
	ext2_file_t efile;
	ext2_filsys e2fs = current_ext2fs();

	debugf("enter");
	debugf("path = %s", path);

	efile = do_open(e2fs, path, fi->flags);
	if (efile == NULL) {
		debugf("do_open(%s); failed", path);
		return -ENOENT;
	}
	fi->fh = (uint64_t) efile;

	debugf("leave");
	return 0;
}
