/**
 * Copyright (c) 2008 Alper Akcan
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

int op_chmod (const char *path, mode_t mode)
{
	int rt;
	int mask;
	errcode_t rc;
	ext2_ino_t ino;
	struct ext2_inode inode;

	debugf("enter");
	debugf("path = %s 0%o", path, mode);
	
	rt = do_readinode(path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", path);
		return rt;
	}
	
	mask = S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX;
	inode.i_mode = (inode.i_mode & ~mask) | (mode & mask);

	rc = ext2fs_write_inode(priv.fs, ino, &inode);
	if (rc) {
		debugf("ext2fs_write_inode(priv.fs, ino, &inode); failed");
		return -EIO;
	}

	debugf("leave");
	return 0;
}
