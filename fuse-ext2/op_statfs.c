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

static int test_root (unsigned int a, unsigned int b)
{
	while (1) {
		if (a < b) {
			return 0;
		}
		if (a == b) {
			return 1;
		}
		if (a % b) {
			return 0;
		}
		a = a / b;
	}
}

static int ext2_group_spare (int group)
{
	if (group <= 1) {
		return 1;
	}
	return (test_root(group, 3) || test_root(group, 5) || test_root(group, 7));
}

static int ext2_bg_has_super (ext2_filsys e2fs, int group)
{
	if (EXT2_HAS_RO_COMPAT_FEATURE(e2fs->super, EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER) &&
	    !ext2_group_spare(group)) {
		return 0;
	}
	return 1;
}

static int ext2_bg_num_gdb (ext2_filsys e2fs, int group)
{
	if (EXT2_HAS_RO_COMPAT_FEATURE(e2fs->super, EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER) &&
	    !ext2_group_spare(group)) {
		return 0;
	}
	return 1;
}

#define EXT2_BLOCKS_COUNT(s) ((s)->s_blocks_count | ((__u64) (s)->s_blocks_count_hi << 32))
#define EXT2_RBLOCKS_COUNT(s) ((s)->s_r_blocks_count | ((__u64) (s)->s_r_blocks_count_hi << 32))
#define EXT2_FBLOCKS_COUNT(s) ((s)->s_free_blocks_count | ((__u64) (s)->s_free_blocks_hi << 32))

int op_statfs (const char *path, struct statvfs *buf)
{
	unsigned long long i;
	unsigned long long s_gdb_count = 0;
	unsigned long long s_groups_count = 0;
	unsigned long long s_itb_per_group = 0;
	unsigned long long s_overhead_last = 0;
	unsigned long long s_inodes_per_block = 0;

	ext2_filsys e2fs = current_ext2fs();

	debugf("enter");

	memset(buf, 0, sizeof(struct statvfs));

	if (e2fs->super->s_default_mount_opts & EXT2_MOUNT_MINIX_DF) {
		s_overhead_last = 0;
	} else {
		s_overhead_last = e2fs->super->s_first_data_block;
		s_groups_count = ((EXT2_BLOCKS_COUNT(e2fs->super) - e2fs->super->s_first_data_block - 1) / e2fs->super->s_blocks_per_group) + 1;
		s_gdb_count = (s_groups_count + EXT2_DESC_PER_BLOCK(e2fs->super) - 1) / EXT2_DESC_PER_BLOCK(e2fs->super);
		for (i = 0; i < s_groups_count; i++) {
			s_overhead_last += ext2_bg_has_super(e2fs, i) + ((ext2_bg_num_gdb(e2fs, i) == 0) ? 0 : s_gdb_count);
		}
		s_inodes_per_block = EXT2_BLOCK_SIZE(e2fs->super) / EXT2_INODE_SIZE(e2fs->super);
		s_itb_per_group = e2fs->super->s_inodes_per_group / s_inodes_per_block;
		s_overhead_last += (s_groups_count * (2 +  s_itb_per_group));
	}
	buf->f_bsize = EXT2_BLOCK_SIZE(e2fs->super);
	buf->f_frsize = EXT2_FRAG_SIZE(e2fs->super);
	buf->f_blocks = EXT2_BLOCKS_COUNT(e2fs->super) - s_overhead_last;
	buf->f_bfree = EXT2_FBLOCKS_COUNT(e2fs->super);
	if (EXT2_FBLOCKS_COUNT(e2fs->super) < EXT2_RBLOCKS_COUNT(e2fs->super)) {
		buf->f_bavail = 0;
	} else {
		buf->f_bavail = EXT2_FBLOCKS_COUNT(e2fs->super) - EXT2_RBLOCKS_COUNT(e2fs->super);
	}
	buf->f_files = e2fs->super->s_inodes_count;
	buf->f_ffree = e2fs->super->s_free_inodes_count;
	buf->f_favail = e2fs->super->s_free_inodes_count;
	buf->f_namemax = EXT2_NAME_LEN;

	debugf("leave");
	return 0;
}
