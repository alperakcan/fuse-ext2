/*
 * csum.c --- checksumming of ext3 structures
 *
 * Copyright (C) 2006 Cluster File Systems, Inc.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "ext2_fs.h"
#include "ext2fs.h"
#include "crc16.h"
#include <assert.h>

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

STATIC __u16 ext2fs_group_desc_csum(ext2_filsys fs, dgrp_t group)
{
	__u16 crc = 0;
	struct ext2_group_desc *desc;

	desc = &fs->group_desc[group];

	if (fs->super->s_feature_ro_compat & EXT4_FEATURE_RO_COMPAT_GDT_CSUM) {
		int offset = offsetof(struct ext2_group_desc, bg_checksum);

#ifdef WORDS_BIGENDIAN
		struct ext2_group_desc swabdesc = *desc;

		/* Have to swab back to little-endian to do the checksum */
		ext2fs_swap_group_desc(&swabdesc);
		desc = &swabdesc;

		group = ext2fs_swab32(group);
#endif
		crc = ext2fs_crc16(~0, fs->super->s_uuid,
				   sizeof(fs->super->s_uuid));
		crc = ext2fs_crc16(crc, &group, sizeof(group));
		crc = ext2fs_crc16(crc, desc, offset);
		offset += sizeof(desc->bg_checksum); /* skip checksum */
		assert(offset == sizeof(*desc));
		/* for checksum of struct ext4_group_desc do the rest...*/
		if (offset < fs->super->s_desc_size) {
			crc = ext2fs_crc16(crc, (char *)desc + offset,
				    fs->super->s_desc_size - offset);
		}
	}

	return crc;
}

int ext2fs_group_desc_csum_verify(ext2_filsys fs, dgrp_t group)
{
	if (EXT2_HAS_RO_COMPAT_FEATURE(fs->super,
				       EXT4_FEATURE_RO_COMPAT_GDT_CSUM) &&
	    (fs->group_desc[group].bg_checksum !=
	     ext2fs_group_desc_csum(fs, group)))
		return 0;

	return 1;
}

void ext2fs_group_desc_csum_set(ext2_filsys fs, dgrp_t group)
{
	if (EXT2_HAS_RO_COMPAT_FEATURE(fs->super,
				       EXT4_FEATURE_RO_COMPAT_GDT_CSUM))
		fs->group_desc[group].bg_checksum =
			ext2fs_group_desc_csum(fs, group);
}

static __u32 find_last_inode_ingrp(ext2fs_inode_bitmap bitmap,
				   __u32 inodes_per_grp, dgrp_t grp_no)
{
	ext2_ino_t i, start_ino, end_ino;

	start_ino = grp_no * inodes_per_grp + 1;
	end_ino = start_ino + inodes_per_grp - 1;

	for (i = end_ino; i >= start_ino; i--) {
		if (ext2fs_fast_test_inode_bitmap(bitmap, i))
			return i - start_ino + 1;
	}
	return inodes_per_grp;
}

/* update the bitmap flags, set the itable high watermark, and calculate
 * checksums for the group descriptors */
errcode_t ext2fs_set_gdt_csum(ext2_filsys fs)
{
	struct ext2_super_block *sb = fs->super;
	struct ext2_group_desc *bg = fs->group_desc;
	int dirty = 0;
	dgrp_t i;

	if (!fs->inode_map)
		return EXT2_ET_NO_INODE_BITMAP;

	if (!EXT2_HAS_RO_COMPAT_FEATURE(fs->super,
					EXT4_FEATURE_RO_COMPAT_GDT_CSUM))
		return 0;

	for (i = 0; i < fs->group_desc_count; i++, bg++) {
		int old_csum = bg->bg_checksum;
		int old_unused = bg->bg_itable_unused;
		int old_flags = bg->bg_flags;

		if (bg->bg_free_inodes_count == sb->s_inodes_per_group) {
			bg->bg_flags |= EXT2_BG_INODE_UNINIT;
			bg->bg_itable_unused = sb->s_inodes_per_group;
		} else {
			bg->bg_flags &= ~EXT2_BG_INODE_UNINIT;
			bg->bg_itable_unused = sb->s_inodes_per_group -
				find_last_inode_ingrp(fs->inode_map,
						      sb->s_inodes_per_group,i);
		}

		ext2fs_group_desc_csum_set(fs, i);
		if (old_flags != bg->bg_flags)
			dirty = 1;
		if (old_unused != bg->bg_itable_unused)
			dirty = 1;
		if (old_csum != bg->bg_checksum)
			dirty = 1;
	}
	if (dirty)
		ext2fs_mark_super_dirty(fs);
	return 0;
}
