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

static inline dev_t old_decode_dev (__u16 val)
{
	return makedev((val >> 8) & 255, val & 255);
}

static inline dev_t new_decode_dev (__u32 dev)
{
	unsigned major = (dev & 0xfff00) >> 8;
	unsigned minor = (dev & 0xff) | ((dev >> 12) & 0xfff00);
	return makedev(major, minor);
}

void do_fillstatbuf (ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode *inode, struct stat *st)
{
	debugf("enter");
	memset(st, 0, sizeof(*st));
	/* XXX workaround
	 * should be unique and != existing devices */
	st->st_dev = (dev_t) ((long) e2fs);
	st->st_ino = ino;
	st->st_mode = inode->i_mode;
	st->st_nlink = inode->i_links_count;
	st->st_uid = inode->i_uid;	/* add in uid_high */
	st->st_gid = inode->i_gid;	/* add in gid_high */
	if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode)) {
		if (inode->i_block[0]) {
			st->st_rdev = old_decode_dev(ext2fs_le32_to_cpu(inode->i_block[0]));
		} else {
			st->st_rdev = new_decode_dev(ext2fs_le32_to_cpu(inode->i_block[1]));
		}
	} else {
		st->st_rdev = 0;
	}
	st->st_size = EXT2_I_SIZE(inode);
	st->st_blksize = EXT2_BLOCK_SIZE(e2fs->super);
	st->st_blocks = inode->i_blocks;
	st->st_atime = inode->i_atime;
	st->st_mtime = inode->i_mtime;
	st->st_ctime = inode->i_ctime;
#if __FreeBSD__ == 10
	st->st_gen = inode->i_generation;
#endif
	debugf("leave");
}
