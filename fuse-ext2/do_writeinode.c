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

int do_writeinode (ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode *inode)
{
	int rt;
	errcode_t rc;
	if (inode->i_links_count < 1) {
		rt = do_killfilebyinode(e2fs, ino, inode);
		if (rt) {
			debugf("do_killfilebyinode(e2fs, ino, inode); failed");
			return rt;
		}
	} else {
		rc = ext2fs_write_inode(e2fs, ino, inode);
		if (rc) {
			debugf("ext2fs_read_inode(e2fs, *ino, inode); failed");
			return -EIO;
		}
	}
	return 0;
}
