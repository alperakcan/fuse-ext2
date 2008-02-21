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

void do_fillstatbuf (ext2_ino_t ino, struct ext2_inode *inode, struct stat *st)
{
	debugf("enter");
	memset(st, 0, sizeof(*st));
	/* st_dev */
	st->st_ino = ino;
	st->st_mode = inode->i_mode;
	st->st_nlink = inode->i_links_count;
	st->st_uid = inode->i_uid;	/* add in uid_high */
	st->st_gid = inode->i_gid;	/* add in gid_high */
	/* st_rdev */
	st->st_size = inode->i_size;
	st->st_blksize = EXT2_BLOCK_SIZE(priv.fs->super);
	st->st_blocks = inode->i_blocks;
	st->st_atime = inode->i_atime;
	st->st_mtime = inode->i_mtime;
	st->st_ctime = inode->i_ctime;
	st->st_gen = inode->i_generation;
	debugf("leave");
}
