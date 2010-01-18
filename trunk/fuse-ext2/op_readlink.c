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

int op_readlink (const char *path, char *buf, size_t size)
{
	int rt;
	size_t s;
	errcode_t rc;
	ext2_ino_t ino;
	char *b = NULL;
	char *pathname;
	struct ext2_inode inode;
	ext2_filsys e2fs = current_ext2fs();
	
	debugf("enter");
	debugf("path = %s", path);

	rt = do_readinode(e2fs, path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", path);
		return rt;
	}
	
	if (!LINUX_S_ISLNK(inode.i_mode)) {
		debugf("%s is not a link", path);
		return -EINVAL;
	}
	
	if (ext2fs_inode_data_blocks(e2fs, &inode)) {
		rc = ext2fs_get_mem(EXT2_BLOCK_SIZE(e2fs->super), &b);
		if (rc) {
			debugf("ext2fs_get_mem(EXT2_BLOCK_SIZE(e2fs->super), &b); failed");
			return -ENOMEM;
		}
		rc = io_channel_read_blk(e2fs->io, inode.i_block[0], 1, b);
		if (rc) {
			ext2fs_free_mem(&b);
			debugf("io_channel_read_blk(e2fs->io, inode.i_block[0], 1, b); failed");
			return -EIO;
		}
		pathname = b;
	} else {
		pathname = (char *) &(inode.i_block[0]);
	}
	
	debugf("pathname: %s", pathname);
	
	s = (size < strlen(pathname) + 1) ? size : strlen(pathname) + 1;
	snprintf(buf, s, "%s", pathname);
	
	if (b) {
		ext2fs_free_mem(&b);
	}

	debugf("leave");
	return 0;
}
