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

static int release_blocks_proc (ext2_filsys fs, blk_t *blocknr, int blockcnt, void *private)
{
	blk_t block;

	debugf("enter");

	block = *blocknr;
	ext2fs_block_alloc_stats(fs, block, -1);

	debugf("leave");
#ifdef CLEAN_UNUSED_BLOCKS
	*blocknr = 0;
	return BLOCK_CHANGED;
#else
	return 0;
#endif
}

int do_killfilebyinode (ext2_filsys e2fs, ext2_ino_t ino, struct ext2_inode *inode)
{
	errcode_t rc;
	char scratchbuf[3*e2fs->blocksize];

	debugf("enter");

	inode->i_links_count = 0;
	inode->i_dtime = time(NULL);

	rc = ext2fs_write_inode(e2fs, ino, inode);
	if (rc) {
		debugf("ext2fs_write_inode(e2fs, ino, inode); failed");
		return -EIO;
	}

	if (ext2fs_inode_has_valid_blocks(inode)) {
		debugf("start block delete for %d", ino);
#ifdef CLEAN_UNUSED_BLOCKS
		ext2fs_block_iterate(e2fs, ino, BLOCK_FLAG_DEPTH_TRAVERSE, scratchbuf, release_blocks_proc, NULL);
#else
		ext2fs_block_iterate(e2fs, ino, 0, scratchbuf, release_blocks_proc, NULL);
#endif
	}

	ext2fs_inode_alloc_stats2(e2fs, ino, -1, LINUX_S_ISDIR(inode->i_mode));

	debugf("leave");
	return 0;
}
