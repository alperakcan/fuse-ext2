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

static int test_root (int a, int b)
{
	int num = b;

	while (a < num) {
		num *= b;
	}

	return num == a;
}

static int ext2_group_spare (int group)
{
	if (group <= 1) {
		return 1;
	}
	return (test_root(group, 3) || test_root(group, 5) || test_root(group, 7));
}

static int ext2_bg_has_super (int group)
{
	if (EXT2_HAS_RO_COMPAT_FEATURE(priv.fs->super, EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER) &&
	    !ext2_group_spare(group)) {
		return 0;
	}
	return 1;
}

static int ext2_bg_num_gdb (int group)
{
	if (EXT2_HAS_RO_COMPAT_FEATURE(priv.fs->super, EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER) &&
	    !ext2_group_spare(group)) {
		return 0;
	}
	return 1;
}

int op_statfs(const char *path, struct statvfs *buf)
{
	unsigned long i;
	unsigned long s_gdb_count = 0;
	unsigned long s_groups_count = 0;
	unsigned long s_itb_per_group = 0;
	unsigned long s_overhead_last = 0;
	unsigned long s_inodes_per_block = 0;

	debugf("enter");

	memset(buf, 0, sizeof(struct statvfs));

	if (priv.fs->super->s_default_mount_opts & EXT2_MOUNT_MINIX_DF) {
		s_overhead_last = 0;
	} else {
		s_overhead_last = priv.fs->super->s_first_data_block;
		s_groups_count = ((priv.fs->super->s_blocks_count - priv.fs->super->s_first_data_block - 1) / priv.fs->super->s_blocks_per_group) + 1;
		s_gdb_count = (s_groups_count + EXT2_DESC_PER_BLOCK(priv.fs->super) - 1) / EXT2_DESC_PER_BLOCK(priv.fs->super);
		for (i = 0; i < s_groups_count; i++) {
			s_overhead_last += ext2_bg_has_super(i) + ((ext2_bg_num_gdb(i) == 0) ? 0 : s_gdb_count);
		}
		s_inodes_per_block = EXT2_BLOCK_SIZE(priv.fs->super) / EXT2_INODE_SIZE(priv.fs->super);
		s_itb_per_group = priv.fs->super->s_inodes_per_group / s_inodes_per_block;
		s_overhead_last += (s_groups_count * (2 +  s_itb_per_group));
	}
	buf->f_bsize = EXT2_BLOCK_SIZE(priv.fs->super);
	buf->f_frsize = EXT2_FRAG_SIZE(priv.fs->super);
	buf->f_blocks = priv.fs->super->s_blocks_count - s_overhead_last;
	buf->f_bfree = priv.fs->super->s_free_blocks_count;
	if (priv.fs->super->s_free_blocks_count < priv.fs->super->s_r_blocks_count) {
		buf->f_bavail = 0;
	} else {
		buf->f_bavail = priv.fs->super->s_free_blocks_count - priv.fs->super->s_r_blocks_count;
	}
	buf->f_files = priv.fs->super->s_inodes_count;
	buf->f_ffree = priv.fs->super->s_free_inodes_count;
	buf->f_favail = priv.fs->super->s_free_inodes_count;
	buf->f_namemax = EXT2_NAME_LEN;

	debugf("leave");
	return 0;
}
