/*
 * i_block.c --- Manage the i_block field for i_blocks
 *
 * Copyright (C) 2008 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */

#include <stdio.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <time.h>
#include <string.h>
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "ext2_fs.h"
#include "ext2fs.h"

errcode_t ext2fs_iblk_add_blocks(ext2_filsys fs, struct ext2_inode *inode,
				 blk64_t num_blocks)
{
	unsigned long long b;

	if ((fs->super->s_feature_ro_compat &
	     EXT4_FEATURE_RO_COMPAT_HUGE_FILE) &&
	    (inode->i_flags & EXT4_HUGE_FILE_FL)) {
		b = inode->i_blocks +
			(((long long) inode->osd2.linux2.l_i_blocks_hi) << 32);
		b += num_blocks;
		inode->i_blocks = b & 0xFFFFFFFF;
		inode->osd2.linux2.l_i_blocks_hi = b >> 32;
	} else
		inode->i_blocks += (fs->blocksize / 512) * num_blocks;
	return 0;
}


errcode_t ext2fs_iblk_sub_blocks(ext2_filsys fs, struct ext2_inode *inode,
				 blk64_t num_blocks)
{
	unsigned long long b;

	if ((fs->super->s_feature_ro_compat &
	     EXT4_FEATURE_RO_COMPAT_HUGE_FILE) &&
	    (inode->i_flags & EXT4_HUGE_FILE_FL)) {
		b = inode->i_blocks +
			(((long long) inode->osd2.linux2.l_i_blocks_hi) << 32);
		b -= num_blocks;
		inode->i_blocks = b & 0xFFFFFFFF;
		inode->osd2.linux2.l_i_blocks_hi = b >> 32;
	} else
		inode->i_blocks -= (fs->blocksize / 512) * num_blocks;
	return 0;
}

errcode_t ext2fs_iblk_set(ext2_filsys fs, struct ext2_inode *inode, blk64_t b)
{
	if ((fs->super->s_feature_ro_compat &
	     EXT4_FEATURE_RO_COMPAT_HUGE_FILE) &&
	    (inode->i_flags & EXT4_HUGE_FILE_FL)) {
		inode->i_blocks = b & 0xFFFFFFFF;
		inode->osd2.linux2.l_i_blocks_hi = b >> 32;
	} else
		inode->i_blocks = (fs->blocksize / 512) * b;
	return 0;
}
