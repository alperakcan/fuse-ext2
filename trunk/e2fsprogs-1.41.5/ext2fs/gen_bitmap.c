/*
 * gen_bitmap.c --- Generic (32-bit) bitmap routines
 *
 * Copyright (C) 2001 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */


#include <stdio.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <time.h>
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "ext2_fs.h"
#include "ext2fs.h"

struct ext2fs_struct_generic_bitmap {
	errcode_t	magic;
	ext2_filsys 	fs;
	__u32		start, end;
	__u32		real_end;
	char	*	description;
	char	*	bitmap;
	errcode_t	base_error_code;
	__u32		reserved[7];
};

/*
 * Used by previously inlined function, so we have to export this and
 * not change the function signature
 */
void ext2fs_warn_bitmap2(ext2fs_generic_bitmap bitmap,
			    int code, unsigned long arg)
{
#ifndef OMIT_COM_ERR
	if (bitmap->description)
		com_err(0, bitmap->base_error_code+code,
			"#%lu for %s", arg, bitmap->description);
	else
		com_err(0, bitmap->base_error_code + code, "#%lu", arg);
#endif
}

static errcode_t check_magic(ext2fs_generic_bitmap bitmap)
{
	if (!bitmap || !((bitmap->magic == EXT2_ET_MAGIC_GENERIC_BITMAP) ||
			 (bitmap->magic == EXT2_ET_MAGIC_INODE_BITMAP) ||
			 (bitmap->magic == EXT2_ET_MAGIC_BLOCK_BITMAP)))
		return EXT2_ET_MAGIC_GENERIC_BITMAP;
	return 0;
}

errcode_t ext2fs_make_generic_bitmap(errcode_t magic, ext2_filsys fs,
				     __u32 start, __u32 end, __u32 real_end,
				     const char *descr, char *init_map,
				     ext2fs_generic_bitmap *ret)
{
	ext2fs_generic_bitmap	bitmap;
	errcode_t		retval;
	size_t			size;

	retval = ext2fs_get_mem(sizeof(struct ext2fs_struct_generic_bitmap),
				&bitmap);
	if (retval)
		return retval;

	bitmap->magic = magic;
	bitmap->fs = fs;
	bitmap->start = start;
	bitmap->end = end;
	bitmap->real_end = real_end;
	switch (magic) {
	case EXT2_ET_MAGIC_INODE_BITMAP:
		bitmap->base_error_code = EXT2_ET_BAD_INODE_MARK;
		break;
	case EXT2_ET_MAGIC_BLOCK_BITMAP:
		bitmap->base_error_code = EXT2_ET_BAD_BLOCK_MARK;
		break;
	default:
		bitmap->base_error_code = EXT2_ET_BAD_GENERIC_MARK;
	}
	if (descr) {
		retval = ext2fs_get_mem(strlen(descr)+1, &bitmap->description);
		if (retval) {
			ext2fs_free_mem(&bitmap);
			return retval;
		}
		strcpy(bitmap->description, descr);
	} else
		bitmap->description = 0;

	size = (size_t) (((bitmap->real_end - bitmap->start) / 8) + 1);
	retval = ext2fs_get_mem(size, &bitmap->bitmap);
	if (retval) {
		ext2fs_free_mem(&bitmap->description);
		ext2fs_free_mem(&bitmap);
		return retval;
	}

	if (init_map)
		memcpy(bitmap->bitmap, init_map, size);
	else
		memset(bitmap->bitmap, 0, size);
	*ret = bitmap;
	return 0;
}

errcode_t ext2fs_allocate_generic_bitmap(__u32 start,
					 __u32 end,
					 __u32 real_end,
					 const char *descr,
					 ext2fs_generic_bitmap *ret)
{
	return ext2fs_make_generic_bitmap(EXT2_ET_MAGIC_GENERIC_BITMAP, 0,
					  start, end, real_end, descr, 0, ret);
}

errcode_t ext2fs_copy_generic_bitmap(ext2fs_generic_bitmap src,
				     ext2fs_generic_bitmap *dest)
{
	return (ext2fs_make_generic_bitmap(src->magic, src->fs,
					   src->start, src->end,
					   src->real_end,
					   src->description, src->bitmap,
					   dest));
}

void ext2fs_free_generic_bitmap(ext2fs_inode_bitmap bitmap)
{
	if (check_magic(bitmap))
		return;

	bitmap->magic = 0;
	if (bitmap->description) {
		ext2fs_free_mem(&bitmap->description);
		bitmap->description = 0;
	}
	if (bitmap->bitmap) {
		ext2fs_free_mem(&bitmap->bitmap);
		bitmap->bitmap = 0;
	}
	ext2fs_free_mem(&bitmap);
}

int ext2fs_test_generic_bitmap(ext2fs_generic_bitmap bitmap,
					blk_t bitno)
{
	if ((bitno < bitmap->start) || (bitno > bitmap->end)) {
		ext2fs_warn_bitmap2(bitmap, EXT2FS_TEST_ERROR, bitno);
		return 0;
	}
	return ext2fs_test_bit(bitno - bitmap->start, bitmap->bitmap);
}

int ext2fs_mark_generic_bitmap(ext2fs_generic_bitmap bitmap,
					 __u32 bitno)
{
	if ((bitno < bitmap->start) || (bitno > bitmap->end)) {
		ext2fs_warn_bitmap2(bitmap, EXT2FS_MARK_ERROR, bitno);
		return 0;
	}
	return ext2fs_set_bit(bitno - bitmap->start, bitmap->bitmap);
}

int ext2fs_unmark_generic_bitmap(ext2fs_generic_bitmap bitmap,
					   blk_t bitno)
{
	if ((bitno < bitmap->start) || (bitno > bitmap->end)) {
		ext2fs_warn_bitmap2(bitmap, EXT2FS_UNMARK_ERROR, bitno);
		return 0;
	}
	return ext2fs_clear_bit(bitno - bitmap->start, bitmap->bitmap);
}

__u32 ext2fs_get_generic_bitmap_start(ext2fs_generic_bitmap bitmap)
{
	return bitmap->start;
}

__u32 ext2fs_get_generic_bitmap_end(ext2fs_generic_bitmap bitmap)
{
	return bitmap->end;
}

void ext2fs_clear_generic_bitmap(ext2fs_generic_bitmap bitmap)
{
	if (check_magic(bitmap))
		return;

	memset(bitmap->bitmap, 0,
	       (size_t) (((bitmap->real_end - bitmap->start) / 8) + 1));
}

errcode_t ext2fs_fudge_generic_bitmap_end(ext2fs_inode_bitmap bitmap,
					  errcode_t magic, errcode_t neq,
					  ext2_ino_t end, ext2_ino_t *oend)
{
	EXT2_CHECK_MAGIC(bitmap, magic);

	if (end > bitmap->real_end)
		return neq;
	if (oend)
		*oend = bitmap->end;
	bitmap->end = end;
	return 0;
}

errcode_t ext2fs_resize_generic_bitmap(errcode_t magic,
				       __u32 new_end, __u32 new_real_end,
				       ext2fs_generic_bitmap bmap)
{
	errcode_t	retval;
	size_t		size, new_size;
	__u32		bitno;

	if (!bmap || (bmap->magic != magic))
		return magic;

	/*
	 * If we're expanding the bitmap, make sure all of the new
	 * parts of the bitmap are zero.
	 */
	if (new_end > bmap->end) {
		bitno = bmap->real_end;
		if (bitno > new_end)
			bitno = new_end;
		for (; bitno > bmap->end; bitno--)
			ext2fs_clear_bit(bitno - bmap->start, bmap->bitmap);
	}
	if (new_real_end == bmap->real_end) {
		bmap->end = new_end;
		return 0;
	}

	size = ((bmap->real_end - bmap->start) / 8) + 1;
	new_size = ((new_real_end - bmap->start) / 8) + 1;

	if (size != new_size) {
		retval = ext2fs_resize_mem(size, new_size, &bmap->bitmap);
		if (retval)
			return retval;
	}
	if (new_size > size)
		memset(bmap->bitmap + size, 0, new_size - size);

	bmap->end = new_end;
	bmap->real_end = new_real_end;
	return 0;
}

errcode_t ext2fs_compare_generic_bitmap(errcode_t magic, errcode_t neq,
					ext2fs_generic_bitmap bm1,
					ext2fs_generic_bitmap bm2)
{
	blk_t	i;

	if (!bm1 || bm1->magic != magic)
		return magic;
	if (!bm2 || bm2->magic != magic)
		return magic;

	if ((bm1->start != bm2->start) ||
	    (bm1->end != bm2->end) ||
	    (memcmp(bm1->bitmap, bm2->bitmap,
		    (size_t) (bm1->end - bm1->start)/8)))
		return neq;

	for (i = bm1->end - ((bm1->end - bm1->start) % 8); i <= bm1->end; i++)
		if (ext2fs_fast_test_block_bitmap(bm1, i) !=
		    ext2fs_fast_test_block_bitmap(bm2, i))
			return neq;

	return 0;
}

void ext2fs_set_generic_bitmap_padding(ext2fs_generic_bitmap map)
{
	__u32	i, j;

	/* Protect loop from wrap-around if map->real_end is maxed */
	for (i=map->end+1, j = i - map->start;
	     i <= map->real_end && i > map->end;
	     i++, j++)
		ext2fs_set_bit(j, map->bitmap);
}

errcode_t ext2fs_get_generic_bitmap_range(ext2fs_generic_bitmap bmap,
					  errcode_t magic,
					  __u32 start, __u32 num,
					  void *out)
{
	if (!bmap || (bmap->magic != magic))
		return magic;

	if ((start < bmap->start) || (start+num-1 > bmap->real_end))
		return EXT2_ET_INVALID_ARGUMENT;

	memcpy(out, bmap->bitmap + (start >> 3), (num+7) >> 3);
	return 0;
}

errcode_t ext2fs_set_generic_bitmap_range(ext2fs_generic_bitmap bmap,
					  errcode_t magic,
					  __u32 start, __u32 num,
					  void *in)
{
	if (!bmap || (bmap->magic != magic))
		return magic;

	if ((start < bmap->start) || (start+num-1 > bmap->real_end))
		return EXT2_ET_INVALID_ARGUMENT;

	memcpy(bmap->bitmap + (start >> 3), in, (num+7) >> 3);
	return 0;
}

int ext2fs_test_block_bitmap_range(ext2fs_block_bitmap bitmap,
				   blk_t block, int num)
{
	int	i;

	if ((block < bitmap->start) || (block+num-1 > bitmap->real_end)) {
		ext2fs_warn_bitmap(EXT2_ET_BAD_BLOCK_TEST,
				   block, bitmap->description);
		return 0;
	}
	for (i=0; i < num; i++) {
		if (ext2fs_fast_test_block_bitmap(bitmap, block+i))
			return 0;
	}
	return 1;
}

void ext2fs_mark_block_bitmap_range(ext2fs_block_bitmap bitmap,
				    blk_t block, int num)
{
	int	i;

	if ((block < bitmap->start) || (block+num-1 > bitmap->end)) {
		ext2fs_warn_bitmap(EXT2_ET_BAD_BLOCK_MARK, block,
				   bitmap->description);
		return;
	}
	for (i=0; i < num; i++)
		ext2fs_fast_set_bit(block + i - bitmap->start, bitmap->bitmap);
}

void ext2fs_unmark_block_bitmap_range(ext2fs_block_bitmap bitmap,
					       blk_t block, int num)
{
	int	i;

	if ((block < bitmap->start) || (block+num-1 > bitmap->end)) {
		ext2fs_warn_bitmap(EXT2_ET_BAD_BLOCK_UNMARK, block,
				   bitmap->description);
		return;
	}
	for (i=0; i < num; i++)
		ext2fs_fast_clear_bit(block + i - bitmap->start,
				      bitmap->bitmap);
}
