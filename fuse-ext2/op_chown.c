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

#include "fuse-ext2.h"

int op_chown (const char *path, uid_t uid, gid_t gid)
{
	int rt;
	errcode_t rc;
	ext2_ino_t ino;
	struct ext2_inode inode;

	debugf("enter");
	debugf("path = %s", path);
	
	rt = do_readinode(path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", path);
		return rt;
	}
	
	inode.i_gid = gid;
	inode.i_uid = uid;

	rc = ext2fs_write_inode(priv.fs, ino, &inode);
	if (rc) {
		debugf("ext2fs_write_inode(priv.fs, ino, &inode); failed");
		return -EIO;
	}

	debugf("leave");
	return 0;
}
