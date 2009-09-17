/*
 * extent.c --- routines to implement extents support
 *
 * Copyright (C) 2007 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "ext2_fs.h"
#include "ext2fsP.h"
#include "e2image.h"

/*
 * Definitions to be dropped in lib/ext2fs/ext2fs.h
 */

/*
 * Private definitions
 */

struct extent_path {
	char		*buf;
	int		entries;
	int		max_entries;
	int		left;
	int		visit_num;
	int		flags;
	blk64_t		end_blk;
	void		*curr;
};


struct ext2_extent_handle {
	errcode_t		magic;
	ext2_filsys		fs;
	ext2_ino_t 		ino;
	struct ext2_inode	*inode;
	int			type;
	int			level;
	int			max_depth;
	struct extent_path	*path;
};

struct ext2_extent_path {
	errcode_t		magic;
	int			leaf_height;
	blk64_t			lblk;
};

/*
 *  Useful Debugging stuff
 */

#ifdef DEBUG
static void dbg_show_header(struct ext3_extent_header *eh)
{
	printf("header: magic=%x entries=%u max=%u depth=%u generation=%u\n",
			ext2fs_le16_to_cpu(eh->eh_magic),
			ext2fs_le16_to_cpu(eh->eh_entries),
			ext2fs_le16_to_cpu(eh->eh_max),
			ext2fs_le16_to_cpu(eh->eh_depth),
			ext2fs_le32_to_cpu(eh->eh_generation));
}

static void dbg_show_index(struct ext3_extent_idx *ix)
{
	printf("index: block=%u leaf=%u leaf_hi=%u unused=%u\n",
			ext2fs_le32_to_cpu(ix->ei_block),
			ext2fs_le32_to_cpu(ix->ei_leaf),
			ext2fs_le16_to_cpu(ix->ei_leaf_hi),
			ext2fs_le16_to_cpu(ix->ei_unused));
}

static void dbg_show_extent(struct ext3_extent *ex)
{
	printf("extent: block=%u-%u len=%u start=%u start_hi=%u\n",
			ext2fs_le32_to_cpu(ex->ee_block),
			ext2fs_le32_to_cpu(ex->ee_block) +
			ext2fs_le16_to_cpu(ex->ee_len) - 1,
			ext2fs_le16_to_cpu(ex->ee_len),
			ext2fs_le32_to_cpu(ex->ee_start),
			ext2fs_le16_to_cpu(ex->ee_start_hi));
}

static void dbg_print_extent(char *desc, struct ext2fs_extent *extent)
{
	if (desc)
		printf("%s: ", desc);
	printf("extent: lblk %llu--%llu, len %u, pblk %llu, flags: ",
	       extent->e_lblk, extent->e_lblk + extent->e_len - 1,
	       extent->e_len, extent->e_pblk);
	if (extent->e_flags & EXT2_EXTENT_FLAGS_LEAF)
		fputs("LEAF ", stdout);
	if (extent->e_flags & EXT2_EXTENT_FLAGS_UNINIT)
		fputs("UNINIT ", stdout);
	if (extent->e_flags & EXT2_EXTENT_FLAGS_SECOND_VISIT)
		fputs("2ND_VISIT ", stdout);
	if (!extent->e_flags)
		fputs("(none)", stdout);
	fputc('\n', stdout);

}

#else
#define dbg_show_header(eh) do { } while (0)
#define dbg_show_index(ix) do { } while (0)
#define dbg_show_extent(ex) do { } while (0)
#define dbg_print_extent(desc, ex) do { } while (0)
#endif

/*
 * Verify the extent header as being sane
 */
errcode_t ext2fs_extent_header_verify(void *ptr, int size)
{
	int eh_max, entry_size;
	struct ext3_extent_header *eh = ptr;

	dbg_show_header(eh);
	if (ext2fs_le16_to_cpu(eh->eh_magic) != EXT3_EXT_MAGIC)
		return EXT2_ET_EXTENT_HEADER_BAD;
	if (ext2fs_le16_to_cpu(eh->eh_entries) > ext2fs_le16_to_cpu(eh->eh_max))
		return EXT2_ET_EXTENT_HEADER_BAD;
	if (eh->eh_depth == 0)
		entry_size = sizeof(struct ext3_extent);
	else
		entry_size = sizeof(struct ext3_extent_idx);

	eh_max = (size - sizeof(*eh)) / entry_size;
	/* Allow two extent-sized items at the end of the block, for
	 * ext4_extent_tail with checksum in the future. */
	if ((ext2fs_le16_to_cpu(eh->eh_max) > eh_max) ||
	    (ext2fs_le16_to_cpu(eh->eh_max) < (eh_max - 2)))
		return EXT2_ET_EXTENT_HEADER_BAD;

	return 0;
}


/*
 * Begin functions to handle an inode's extent information
 */
extern void ext2fs_extent_free(ext2_extent_handle_t handle)
{
	int			i;

	if (!handle)
		return;

	if (handle->inode)
		ext2fs_free_mem(&handle->inode);
	if (handle->path) {
		for (i=1; i <= handle->max_depth; i++) {
			if (handle->path[i].buf)
				ext2fs_free_mem(&handle->path[i].buf);
		}
		ext2fs_free_mem(&handle->path);
	}
	ext2fs_free_mem(&handle);
}

extern errcode_t ext2fs_extent_open(ext2_filsys fs, ext2_ino_t ino,
				    ext2_extent_handle_t *ret_handle)
{
	return ext2fs_extent_open2(fs, ino, NULL, ret_handle);
}

extern errcode_t ext2fs_extent_open2(ext2_filsys fs, ext2_ino_t ino,
				    struct ext2_inode *inode,
				    ext2_extent_handle_t *ret_handle)
{
	struct ext2_extent_handle	*handle;
	errcode_t			retval;
	int				i;
	struct ext3_extent_header	*eh;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	if (!inode)
		if ((ino == 0) || (ino > fs->super->s_inodes_count))
			return EXT2_ET_BAD_INODE_NUM;

	retval = ext2fs_get_mem(sizeof(struct ext2_extent_handle), &handle);
	if (retval)
		return retval;
	memset(handle, 0, sizeof(struct ext2_extent_handle));

	retval = ext2fs_get_mem(sizeof(struct ext2_inode), &handle->inode);
	if (retval)
		goto errout;

	handle->ino = ino;
	handle->fs = fs;

	if (inode) {
		memcpy(handle->inode, inode, sizeof(struct ext2_inode));
	}
	else {
		retval = ext2fs_read_inode(fs, ino, handle->inode);
		if (retval)
			goto errout;
	}

	eh = (struct ext3_extent_header *) &handle->inode->i_block[0];

	for (i=0; i < EXT2_N_BLOCKS; i++)
		if (handle->inode->i_block[i])
			break;
	if (i >= EXT2_N_BLOCKS) {
		eh->eh_magic = ext2fs_cpu_to_le16(EXT3_EXT_MAGIC);
		eh->eh_depth = 0;
		eh->eh_entries = 0;
		i = (sizeof(handle->inode->i_block) - sizeof(*eh)) /
			sizeof(struct ext3_extent);
		eh->eh_max = ext2fs_cpu_to_le16(i);
		handle->inode->i_flags |= EXT4_EXTENTS_FL;
	}

	if (!(handle->inode->i_flags & EXT4_EXTENTS_FL)) {
		retval = EXT2_ET_INODE_NOT_EXTENT;
		goto errout;
	}

	retval = ext2fs_extent_header_verify(eh, sizeof(handle->inode->i_block));
	if (retval)
		goto errout;

	handle->max_depth = ext2fs_le16_to_cpu(eh->eh_depth);
	handle->type = ext2fs_le16_to_cpu(eh->eh_magic);

	retval = ext2fs_get_mem(((handle->max_depth+1) *
				 sizeof(struct extent_path)),
				&handle->path);
	memset(handle->path, 0,
	       (handle->max_depth+1) * sizeof(struct extent_path));
	handle->path[0].buf = (char *) handle->inode->i_block;

	handle->path[0].left = handle->path[0].entries =
		ext2fs_le16_to_cpu(eh->eh_entries);
	handle->path[0].max_entries = ext2fs_le16_to_cpu(eh->eh_max);
	handle->path[0].curr = 0;
	handle->path[0].end_blk =
		((((__u64) handle->inode->i_size_high << 32) +
		  handle->inode->i_size + (fs->blocksize - 1))
		 >> EXT2_BLOCK_SIZE_BITS(fs->super));
	handle->path[0].visit_num = 1;
	handle->level = 0;
	handle->magic = EXT2_ET_MAGIC_EXTENT_HANDLE;

	*ret_handle = handle;
	return 0;

errout:
	ext2fs_extent_free(handle);
	return retval;
}

/*
 * This function is responsible for (optionally) moving through the
 * extent tree and then returning the current extent
 */
errcode_t ext2fs_extent_get(ext2_extent_handle_t handle,
			    int flags, struct ext2fs_extent *extent)
{
	struct extent_path	*path, *newpath;
	struct ext3_extent_header	*eh;
	struct ext3_extent_idx		*ix = 0;
	struct ext3_extent		*ex;
	errcode_t			retval;
	blk_t				blk;
	blk64_t				end_blk;
	int				orig_op, op;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	if (!handle->path)
		return EXT2_ET_NO_CURRENT_NODE;

	orig_op = op = flags & EXT2_EXTENT_MOVE_MASK;

retry:
	path = handle->path + handle->level;
	if ((orig_op == EXT2_EXTENT_NEXT) ||
	    (orig_op == EXT2_EXTENT_NEXT_LEAF)) {
		if (handle->level < handle->max_depth) {
			/* interior node */
			if (path->visit_num == 0) {
				path->visit_num++;
				op = EXT2_EXTENT_DOWN;
			} else if (path->left > 0)
				op = EXT2_EXTENT_NEXT_SIB;
			else if (handle->level > 0)
				op = EXT2_EXTENT_UP;
			else
				return EXT2_ET_EXTENT_NO_NEXT;
		} else {
			/* leaf node */
			if (path->left > 0)
				op = EXT2_EXTENT_NEXT_SIB;
			else if (handle->level > 0)
				op = EXT2_EXTENT_UP;
			else
				return EXT2_ET_EXTENT_NO_NEXT;
		}
		if (op != EXT2_EXTENT_NEXT_SIB) {
#ifdef DEBUG_GET_EXTENT
			printf("<<<< OP = %s\n",
			       (op == EXT2_EXTENT_DOWN) ? "down" :
			       ((op == EXT2_EXTENT_UP) ? "up" : "unknown"));
#endif
		}
	}

	if ((orig_op == EXT2_EXTENT_PREV) ||
	    (orig_op == EXT2_EXTENT_PREV_LEAF)) {
		if (handle->level < handle->max_depth) {
			/* interior node */
			if (path->visit_num > 0 ) {
				/* path->visit_num = 0; */
				op = EXT2_EXTENT_DOWN_AND_LAST;
			} else if (path->left < path->entries-1)
				op = EXT2_EXTENT_PREV_SIB;
			else if (handle->level > 0)
				op = EXT2_EXTENT_UP;
			else
				return EXT2_ET_EXTENT_NO_PREV;
		} else {
			/* leaf node */
			if (path->left < path->entries-1)
				op = EXT2_EXTENT_PREV_SIB;
			else if (handle->level > 0)
				op = EXT2_EXTENT_UP;
			else
				return EXT2_ET_EXTENT_NO_PREV;
		}
		if (op != EXT2_EXTENT_PREV_SIB) {
#ifdef DEBUG_GET_EXTENT
			printf("<<<< OP = %s\n",
			       (op == EXT2_EXTENT_DOWN_AND_LAST) ? "down/last" :
			       ((op == EXT2_EXTENT_UP) ? "up" : "unknown"));
#endif
		}
	}

	if (orig_op == EXT2_EXTENT_LAST_LEAF) {
		if ((handle->level < handle->max_depth) &&
		    (path->left == 0))
			op = EXT2_EXTENT_DOWN;
		else
			op = EXT2_EXTENT_LAST_SIB;
#ifdef DEBUG_GET_EXTENT
		printf("<<<< OP = %s\n",
			   (op == EXT2_EXTENT_DOWN) ? "down" : "last_sib");
#endif
	}

	switch (op) {
	case EXT2_EXTENT_CURRENT:
		ix = path->curr;
		break;
	case EXT2_EXTENT_ROOT:
		handle->level = 0;
		path = handle->path + handle->level;
	case EXT2_EXTENT_FIRST_SIB:
		path->left = path->entries;
		path->curr = 0;
	case EXT2_EXTENT_NEXT_SIB:
		if (path->left <= 0)
			return EXT2_ET_EXTENT_NO_NEXT;
		if (path->curr) {
			ix = path->curr;
			ix++;
		} else {
			eh = (struct ext3_extent_header *) path->buf;
			ix = EXT_FIRST_INDEX(eh);
		}
		path->left--;
		path->curr = ix;
		path->visit_num = 0;
		break;
	case EXT2_EXTENT_PREV_SIB:
		if (!path->curr ||
		    path->left+1 >= path->entries)
			return EXT2_ET_EXTENT_NO_PREV;
		ix = path->curr;
		ix--;
		path->curr = ix;
		path->left++;
		if (handle->level < handle->max_depth)
			path->visit_num = 1;
		break;
	case EXT2_EXTENT_LAST_SIB:
		eh = (struct ext3_extent_header *) path->buf;
		path->curr = EXT_LAST_EXTENT(eh);
		ix = path->curr;
		path->left = 0;
		path->visit_num = 0;
		break;
	case EXT2_EXTENT_UP:
		if (handle->level <= 0)
			return EXT2_ET_EXTENT_NO_UP;
		handle->level--;
		path--;
		ix = path->curr;
		if ((orig_op == EXT2_EXTENT_PREV) ||
		    (orig_op == EXT2_EXTENT_PREV_LEAF))
			path->visit_num = 0;
		break;
	case EXT2_EXTENT_DOWN:
	case EXT2_EXTENT_DOWN_AND_LAST:
		if (!path->curr ||(handle->level >= handle->max_depth))
			return EXT2_ET_EXTENT_NO_DOWN;

		ix = path->curr;
		newpath = path + 1;
		if (!newpath->buf) {
			retval = ext2fs_get_mem(handle->fs->blocksize,
						&newpath->buf);
			if (retval)
				return retval;
		}
		blk = ext2fs_le32_to_cpu(ix->ei_leaf) +
			((__u64) ext2fs_le16_to_cpu(ix->ei_leaf_hi) << 32);
		if ((handle->fs->flags & EXT2_FLAG_IMAGE_FILE) &&
		    (handle->fs->io != handle->fs->image_io))
			memset(newpath->buf, 0, handle->fs->blocksize);
		else {
			retval = io_channel_read_blk(handle->fs->io,
						     blk, 1, newpath->buf);
			if (retval)
				return retval;
		}
		handle->level++;

		eh = (struct ext3_extent_header *) newpath->buf;

		retval = ext2fs_extent_header_verify(eh, handle->fs->blocksize);
		if (retval) {
			handle->level--;
			return retval;
		}

		newpath->left = newpath->entries =
			ext2fs_le16_to_cpu(eh->eh_entries);
		newpath->max_entries = ext2fs_le16_to_cpu(eh->eh_max);

		if (path->left > 0) {
			ix++;
			newpath->end_blk = ext2fs_le32_to_cpu(ix->ei_block);
		} else
			newpath->end_blk = path->end_blk;

		path = newpath;
		if (op == EXT2_EXTENT_DOWN) {
			ix = EXT_FIRST_INDEX((struct ext3_extent_header *) eh);
			path->curr = ix;
			path->left = path->entries - 1;
			path->visit_num = 0;
		} else {
			ix = EXT_LAST_INDEX((struct ext3_extent_header *) eh);
			path->curr = ix;
			path->left = 0;
			if (handle->level < handle->max_depth)
				path->visit_num = 1;
		}
#ifdef DEBUG_GET_EXTENT
		printf("Down to level %d/%d, end_blk=%llu\n",
			   handle->level, handle->max_depth,
			   path->end_blk);
#endif
		break;
	default:
		return EXT2_ET_OP_NOT_SUPPORTED;
	}

	if (!ix)
		return EXT2_ET_NO_CURRENT_NODE;

	extent->e_flags = 0;
#ifdef DEBUG_GET_EXTENT
	printf("(Left %d)\n", path->left);
#endif

	if (handle->level == handle->max_depth) {
		ex = (struct ext3_extent *) ix;

		extent->e_pblk = ext2fs_le32_to_cpu(ex->ee_start) +
			((__u64) ext2fs_le16_to_cpu(ex->ee_start_hi) << 32);
		extent->e_lblk = ext2fs_le32_to_cpu(ex->ee_block);
		extent->e_len = ext2fs_le16_to_cpu(ex->ee_len);
		extent->e_flags |= EXT2_EXTENT_FLAGS_LEAF;
		if (extent->e_len > EXT_INIT_MAX_LEN) {
			extent->e_len -= EXT_INIT_MAX_LEN;
			extent->e_flags |= EXT2_EXTENT_FLAGS_UNINIT;
		}
	} else {
		extent->e_pblk = ext2fs_le32_to_cpu(ix->ei_leaf) +
			((__u64) ext2fs_le16_to_cpu(ix->ei_leaf_hi) << 32);
		extent->e_lblk = ext2fs_le32_to_cpu(ix->ei_block);
		if (path->left > 0) {
			ix++;
			end_blk = ext2fs_le32_to_cpu(ix->ei_block);
		} else
			end_blk = path->end_blk;

		extent->e_len = end_blk - extent->e_lblk;
	}
	if (path->visit_num)
		extent->e_flags |= EXT2_EXTENT_FLAGS_SECOND_VISIT;

	if (((orig_op == EXT2_EXTENT_NEXT_LEAF) ||
	     (orig_op == EXT2_EXTENT_PREV_LEAF)) &&
	    (handle->level != handle->max_depth))
		goto retry;

	if ((orig_op == EXT2_EXTENT_LAST_LEAF) &&
	    ((handle->level != handle->max_depth) ||
	     (path->left != 0)))
		goto retry;

	return 0;
}

static errcode_t update_path(ext2_extent_handle_t handle)
{
	blk64_t				blk;
	errcode_t			retval;
	struct ext3_extent_idx		*ix;

	if (handle->level == 0) {
		retval = ext2fs_write_inode(handle->fs, handle->ino,
					    handle->inode);
	} else {
		ix = handle->path[handle->level - 1].curr;
		blk = ext2fs_le32_to_cpu(ix->ei_leaf) +
			((__u64) ext2fs_le16_to_cpu(ix->ei_leaf_hi) << 32);

		retval = io_channel_write_blk(handle->fs->io,
				      blk, 1, handle->path[handle->level].buf);
	}
	return retval;
}

#if 0
errcode_t ext2fs_extent_save_path(ext2_extent_handle_t handle,
				  ext2_extent_path_t *ret_path)
{
	ext2_extent_path_t	save_path;
	struct ext2fs_extent	extent;
	struct ext2_extent_info	info;
	errcode_t		retval;

	retval = ext2fs_extent_get(handle, EXT2_EXTENT_CURRENT, &extent);
	if (retval)
		return retval;

	retval = ext2fs_extent_get_info(handle, &info);
	if (retval)
		return retval;

	retval = ext2fs_get_mem(sizeof(struct ext2_extent_path), &save_path);
	if (retval)
		return retval;
	memset(save_path, 0, sizeof(struct ext2_extent_path));

	save_path->magic = EXT2_ET_MAGIC_EXTENT_PATH;
	save_path->leaf_height = info.max_depth - info.curr_level - 1;
	save_path->lblk = extent.e_lblk;

	*ret_path = save_path;
	return 0;
}

errcode_t ext2fs_extent_free_path(ext2_extent_path_t path)
{
	EXT2_CHECK_MAGIC(path, EXT2_ET_MAGIC_EXTENT_PATH);

	ext2fs_free_mem(&path);
	return 0;
}
#endif

/*
 * Go to the node at leaf_level which contains logical block blk.
 *
 * leaf_level is height from the leaf node level, i.e.
 * leaf_level 0 is at leaf node, leaf_level 1 is 1 above etc.
 *
 * If "blk" has no mapping (hole) then handle is left at last
 * extent before blk.
 */
static errcode_t extent_goto(ext2_extent_handle_t handle,
			     int leaf_level, blk64_t blk)
{
	struct ext2fs_extent	extent;
	errcode_t		retval;

	retval = ext2fs_extent_get(handle, EXT2_EXTENT_ROOT, &extent);
	if (retval) {
		if (retval == EXT2_ET_EXTENT_NO_NEXT)
			retval = EXT2_ET_EXTENT_NOT_FOUND;
		return retval;
	}

	if (leaf_level > handle->max_depth) {
#ifdef DEBUG
		printf("leaf level %d greater than tree depth %d\n",
			leaf_level, handle->max_depth);
#endif
		return EXT2_ET_OP_NOT_SUPPORTED;
	}

#ifdef DEBUG
	printf("goto extent ino %u, level %d, %llu\n", handle->ino,
	       leaf_level, blk);
#endif

#ifdef DEBUG_GOTO_EXTENTS
	dbg_print_extent("root", &extent);
#endif
	while (1) {
		if (handle->max_depth - handle->level == leaf_level) {
			/* block is in this &extent */
			if ((blk >= extent.e_lblk) &&
			    (blk < extent.e_lblk + extent.e_len))
				return 0;
			if (blk < extent.e_lblk) {
				retval = ext2fs_extent_get(handle,
							   EXT2_EXTENT_PREV_SIB,
							   &extent);
				return EXT2_ET_EXTENT_NOT_FOUND;
			}
			retval = ext2fs_extent_get(handle,
						   EXT2_EXTENT_NEXT_SIB,
						   &extent);
			if (retval == EXT2_ET_EXTENT_NO_NEXT)
				return EXT2_ET_EXTENT_NOT_FOUND;
			if (retval)
				return retval;
			continue;
		}

		retval = ext2fs_extent_get(handle, EXT2_EXTENT_NEXT_SIB,
					   &extent);
		if (retval == EXT2_ET_EXTENT_NO_NEXT)
			goto go_down;
		if (retval)
			return retval;

#ifdef DEBUG_GOTO_EXTENTS
		dbg_print_extent("next", &extent);
#endif
		if (blk == extent.e_lblk)
			goto go_down;
		if (blk > extent.e_lblk)
			continue;

		retval = ext2fs_extent_get(handle, EXT2_EXTENT_PREV_SIB,
					   &extent);
		if (retval)
			return retval;

#ifdef DEBUG_GOTO_EXTENTS
		dbg_print_extent("prev", &extent);
#endif

	go_down:
		retval = ext2fs_extent_get(handle, EXT2_EXTENT_DOWN,
					   &extent);
		if (retval)
			return retval;

#ifdef DEBUG_GOTO_EXTENTS
		dbg_print_extent("down", &extent);
#endif
	}
}

errcode_t ext2fs_extent_goto(ext2_extent_handle_t handle,
			     blk64_t blk)
{
	return extent_goto(handle, 0, blk);
}

/*
 * Traverse back up to root fixing parents of current node as needed.
 *
 * If we changed start of first entry in a node, fix parent index start
 * and so on.
 *
 * Safe to call for any position in node; if not at the first entry,
 * will  simply return.
 */
static errcode_t ext2fs_extent_fix_parents(ext2_extent_handle_t handle)
{
	int				retval = 0;
	blk64_t				start;
	struct extent_path		*path;
	struct ext2fs_extent		extent;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	if (!(handle->fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if (!handle->path)
		return EXT2_ET_NO_CURRENT_NODE;

	path = handle->path + handle->level;
	if (!path->curr)
		return EXT2_ET_NO_CURRENT_NODE;

	retval = ext2fs_extent_get(handle, EXT2_EXTENT_CURRENT, &extent);
	if (retval)
		goto done;

	/* modified node's start block */
	start = extent.e_lblk;

	/* traverse up until index not first, or startblk matches, or top */
	while (handle->level > 0 &&
	       (path->left == path->entries - 1)) {
		retval = ext2fs_extent_get(handle, EXT2_EXTENT_UP, &extent);
		if (retval)
			goto done;
		if (extent.e_lblk == start)
			break;
		path = handle->path + handle->level;
		extent.e_len += (extent.e_lblk - start);
		extent.e_lblk = start;
		retval = ext2fs_extent_replace(handle, 0, &extent);
		if (retval)
			goto done;
		update_path(handle);
	}

	/* put handle back to where we started */
	retval = ext2fs_extent_goto(handle, start);
done:
	return retval;
}

errcode_t ext2fs_extent_replace(ext2_extent_handle_t handle,
				int flags EXT2FS_ATTR((unused)),
				struct ext2fs_extent *extent)
{
	struct extent_path		*path;
	struct ext3_extent_idx		*ix;
	struct ext3_extent		*ex;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	if (!(handle->fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if (!handle->path)
		return EXT2_ET_NO_CURRENT_NODE;

	path = handle->path + handle->level;
	if (!path->curr)
		return EXT2_ET_NO_CURRENT_NODE;

#ifdef DEBUG
	printf("extent replace: %u ", handle->ino);
	dbg_print_extent(0, extent);
#endif

	if (handle->level == handle->max_depth) {
		ex = path->curr;

		ex->ee_block = ext2fs_cpu_to_le32(extent->e_lblk);
		ex->ee_start = ext2fs_cpu_to_le32(extent->e_pblk & 0xFFFFFFFF);
		ex->ee_start_hi = ext2fs_cpu_to_le16(extent->e_pblk >> 32);
		if (extent->e_flags & EXT2_EXTENT_FLAGS_UNINIT) {
			if (extent->e_len > EXT_UNINIT_MAX_LEN)
				return EXT2_ET_EXTENT_INVALID_LENGTH;
			ex->ee_len = ext2fs_cpu_to_le16(extent->e_len +
							EXT_INIT_MAX_LEN);
		} else {
			if (extent->e_len > EXT_INIT_MAX_LEN)
				return EXT2_ET_EXTENT_INVALID_LENGTH;
			ex->ee_len = ext2fs_cpu_to_le16(extent->e_len);
		}
	} else {
		ix = path->curr;

		ix->ei_leaf = ext2fs_cpu_to_le32(extent->e_pblk & 0xFFFFFFFF);
		ix->ei_leaf_hi = ext2fs_cpu_to_le16(extent->e_pblk >> 32);
		ix->ei_block = ext2fs_cpu_to_le32(extent->e_lblk);
		ix->ei_unused = 0;
	}
	update_path(handle);
	return 0;
}

/*
 * allocate a new block, move half the current node to it, and update parent
 *
 * handle will be left pointing at original record.
 */
static errcode_t extent_node_split(ext2_extent_handle_t handle)
{
	errcode_t			retval = 0;
	blk_t				new_node_pblk;
	blk64_t				new_node_start;
	blk64_t				orig_lblk;
	blk64_t				goal_blk = 0;
	int				orig_height;
	char				*block_buf = NULL;
	struct ext2fs_extent		extent;
	struct extent_path		*path, *newpath = 0;
	struct ext3_extent_header	*eh, *neweh;
	int				tocopy;
	int				new_root = 0;
	struct ext2_extent_info		info;

	/* basic sanity */
	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	if (!(handle->fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if (!handle->path)
		return EXT2_ET_NO_CURRENT_NODE;

#ifdef DEBUG
	printf("splitting node at level %d\n", handle->level);
#endif
	retval = ext2fs_extent_get(handle, EXT2_EXTENT_CURRENT, &extent);
	if (retval)
		goto done;

	retval = ext2fs_extent_get_info(handle, &info);
	if (retval)
		goto done;

	/* save the position we were originally splitting... */
	orig_height = info.max_depth - info.curr_level;
	orig_lblk = extent.e_lblk;

	/* Is there room in the parent for a new entry? */
	if (handle->level &&
			(handle->path[handle->level - 1].entries >=
			 handle->path[handle->level - 1].max_entries)) {

#ifdef DEBUG
		printf("parent level %d full; splitting it too\n",
							handle->level - 1);
#endif
		/* split the parent */
		retval = ext2fs_extent_get(handle, EXT2_EXTENT_UP, &extent);
		if (retval)
			goto done;
		goal_blk = extent.e_pblk;

		retval = extent_node_split(handle);
		if (retval)
			goto done;

		/* get handle back to our original split position */
		retval = extent_goto(handle, orig_height, orig_lblk);
		if (retval)
			goto done;
	}

	/* At this point, parent should have room for this split */
	path = handle->path + handle->level;
	if (!path->curr)
		return EXT2_ET_NO_CURRENT_NODE;

	/* extent header of the current node we'll split */
	eh = (struct ext3_extent_header *)path->buf;

	/* splitting root level means moving them all out */
	if (handle->level == 0) {
		new_root = 1;
		tocopy = ext2fs_le16_to_cpu(eh->eh_entries);
		retval = ext2fs_get_mem(((handle->max_depth+2) *
					 sizeof(struct extent_path)),
					&newpath);
		if (retval)
			goto done;
		memset(newpath, 0,
		       ((handle->max_depth+2) * sizeof(struct extent_path)));
	} else {
		tocopy = ext2fs_le16_to_cpu(eh->eh_entries) / 2;
	}

#ifdef DEBUG
	printf("will copy out %d of %d entries at level %d\n",
				tocopy, ext2fs_le16_to_cpu(eh->eh_entries),
				handle->level);
#endif

	if (!tocopy) {
#ifdef DEBUG
		printf("Nothing to copy to new block!\n");
#endif
		retval = EXT2_ET_CANT_SPLIT_EXTENT;
		goto done;
	}

	/* first we need a new block, or can do nothing. */
	block_buf = malloc(handle->fs->blocksize);
	if (!block_buf) {
		retval = ENOMEM;
		goto done;
	}

	if (!goal_blk) {
		dgrp_t	group = ext2fs_group_of_ino(handle->fs, handle->ino);
		__u8	log_flex = handle->fs->super->s_log_groups_per_flex;

		if (log_flex)
			group = group & ~((1 << (log_flex)) - 1);
		goal_blk = (group * handle->fs->super->s_blocks_per_group) +
			handle->fs->super->s_first_data_block;
	}
	retval = ext2fs_alloc_block(handle->fs, (blk_t) goal_blk, block_buf,
				    &new_node_pblk);
	if (retval)
		goto done;

#ifdef DEBUG
	printf("will copy to new node at block %lu\n",
	       (unsigned long) new_node_pblk);
#endif

	/* Copy data into new block buffer */
	/* First the header for the new block... */
	neweh = (struct ext3_extent_header *) block_buf;
	memcpy(neweh, eh, sizeof(struct ext3_extent_header));
	neweh->eh_entries = ext2fs_cpu_to_le16(tocopy);
	neweh->eh_max = ext2fs_cpu_to_le16((handle->fs->blocksize -
			 sizeof(struct ext3_extent_header)) /
				sizeof(struct ext3_extent));

	/* then the entries for the new block... */
	memcpy(EXT_FIRST_INDEX(neweh),
		EXT_FIRST_INDEX(eh) +
			(ext2fs_le16_to_cpu(eh->eh_entries) - tocopy),
		sizeof(struct ext3_extent_idx) * tocopy);

	new_node_start = ext2fs_le32_to_cpu(EXT_FIRST_INDEX(neweh)->ei_block);

	/* ...and write the new node block out to disk. */
	retval = io_channel_write_blk(handle->fs->io, new_node_pblk, 1, block_buf);

	if (retval)
		goto done;

	/* OK! we've created the new node; now adjust the tree */

	/* current path now has fewer active entries, we copied some out */
	if (handle->level == 0) {
		memcpy(newpath, path,
		       sizeof(struct extent_path) * (handle->max_depth+1));
		handle->path = newpath;
		newpath = path;
		path = handle->path;
		path->entries = 1;
		path->left = path->max_entries - 1;
		handle->max_depth++;
		eh->eh_depth = ext2fs_cpu_to_le16(handle->max_depth);
	} else {
		path->entries -= tocopy;
		path->left -= tocopy;
	}

	eh->eh_entries = ext2fs_cpu_to_le16(path->entries);
	/* this writes out the node, incl. the modified header */
	retval = update_path(handle);
	if (retval)
		goto done;

	/* now go up and insert/replace index for new node we created */
	if (new_root) {
		retval = ext2fs_extent_get(handle, EXT2_EXTENT_FIRST_SIB, &extent);
		if (retval)
			goto done;

		extent.e_lblk = new_node_start;
		extent.e_pblk = new_node_pblk;
		extent.e_len = handle->path[0].end_blk - extent.e_lblk;
		retval = ext2fs_extent_replace(handle, 0, &extent);
		if (retval)
			goto done;
	} else {
		__u32 new_node_length;

		retval = ext2fs_extent_get(handle, EXT2_EXTENT_UP, &extent);
		/* will insert after this one; it's length is shorter now */
		new_node_length = new_node_start - extent.e_lblk;
		extent.e_len -= new_node_length;
		retval = ext2fs_extent_replace(handle, 0, &extent);
		if (retval)
			goto done;

		/* now set up the new extent and insert it */
		extent.e_lblk = new_node_start;
		extent.e_pblk = new_node_pblk;
		extent.e_len = new_node_length;
		retval = ext2fs_extent_insert(handle, EXT2_EXTENT_INSERT_AFTER, &extent);
		if (retval)
			goto done;
	}

	/* get handle back to our original position */
	retval = extent_goto(handle, orig_height, orig_lblk);
	if (retval)
		goto done;

	/* new node hooked in, so update inode block count (do this here?) */
	handle->inode->i_blocks += handle->fs->blocksize / 512;
	retval = ext2fs_write_inode(handle->fs, handle->ino,
				    handle->inode);
	if (retval)
		goto done;

done:
	if (newpath)
		ext2fs_free_mem(&newpath);
	free(block_buf);

	return retval;
}

errcode_t ext2fs_extent_insert(ext2_extent_handle_t handle, int flags,
				      struct ext2fs_extent *extent)
{
	struct extent_path		*path;
	struct ext3_extent_idx		*ix;
	struct ext3_extent_header	*eh;
	errcode_t			retval;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	if (!(handle->fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if (!handle->path)
		return EXT2_ET_NO_CURRENT_NODE;

#ifdef DEBUG
	printf("extent insert: %u ", handle->ino);
	dbg_print_extent(0, extent);
#endif

	path = handle->path + handle->level;

	if (path->entries >= path->max_entries) {
		if (flags & EXT2_EXTENT_INSERT_NOSPLIT) {
			return EXT2_ET_CANT_INSERT_EXTENT;
		} else {
#ifdef DEBUG
			printf("node full (level %d) - splitting\n",
				   handle->level);
#endif
			retval = extent_node_split(handle);
			if (retval)
				return retval;
			path = handle->path + handle->level;
		}
	}

	eh = (struct ext3_extent_header *) path->buf;
	if (path->curr) {
		ix = path->curr;
		if (flags & EXT2_EXTENT_INSERT_AFTER) {
			ix++;
			path->left--;
		}
	} else
		ix = EXT_FIRST_INDEX(eh);

	path->curr = ix;

	if (path->left >= 0)
		memmove(ix + 1, ix,
			(path->left+1) * sizeof(struct ext3_extent_idx));
	path->left++;
	path->entries++;

	eh = (struct ext3_extent_header *) path->buf;
	eh->eh_entries = ext2fs_cpu_to_le16(path->entries);

	retval = ext2fs_extent_replace(handle, 0, extent);
	if (retval)
		goto errout;

	retval = update_path(handle);
	if (retval)
		goto errout;

	return 0;

errout:
	ext2fs_extent_delete(handle, 0);
	return retval;
}

/*
 * Sets the physical block for a logical file block in the extent tree.
 *
 * May: map unmapped, unmap mapped, or remap mapped blocks.
 *
 * Mapping an unmapped block adds a single-block extent.
 *
 * Unmapping first or last block modifies extent in-place
 *  - But may need to fix parent's starts too in first-block case
 *
 * Mapping any unmapped block requires adding a (single-block) extent
 * and inserting into proper point in tree.
 *
 * Modifying (unmapping or remapping) a block in the middle
 * of an extent requires splitting the extent.
 *  - Remapping case requires new single-block extent.
 *
 * Remapping first or last block adds an extent.
 *
 * We really need extent adding to be smart about merging.
 */

errcode_t ext2fs_extent_set_bmap(ext2_extent_handle_t handle,
				 blk64_t logical, blk64_t physical, int flags)
{
	errcode_t		ec, retval = 0;
	int			mapped = 1; /* logical is mapped? */
	int			orig_height;
	int			extent_uninit = 0;
	int			prev_uninit = 0;
	int			next_uninit = 0;
	int			new_uninit = 0;
	int			max_len = EXT_INIT_MAX_LEN;
	int			has_prev, has_next;
	blk64_t			orig_lblk;
	struct extent_path	*path;
	struct ext2fs_extent	extent, next_extent, prev_extent;
	struct ext2fs_extent	newextent;
	struct ext2_extent_info	info;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

#ifdef DEBUG
	printf("set_bmap ino %u log %lld phys %lld flags %d\n",
	       handle->ino, logical, physical, flags);
#endif

	if (!(handle->fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if (!handle->path)
		return EXT2_ET_NO_CURRENT_NODE;

	path = handle->path + handle->level;

	if (flags & EXT2_EXTENT_SET_BMAP_UNINIT) {
		new_uninit = 1;
		max_len = EXT_UNINIT_MAX_LEN;
	}

	/* if (re)mapping, set up new extent to insert */
	if (physical) {
		newextent.e_len = 1;
		newextent.e_pblk = physical;
		newextent.e_lblk = logical;
		newextent.e_flags = EXT2_EXTENT_FLAGS_LEAF;
		if (new_uninit)
			newextent.e_flags |= EXT2_EXTENT_FLAGS_UNINIT;
	}

	/* special case if the extent tree is completely empty */
	if ((handle->max_depth == 0) && (path->entries == 0)) {
		retval = ext2fs_extent_insert(handle, 0, &newextent);
		return retval;
	}

	/* save our original location in the extent tree */
	if ((retval = ext2fs_extent_get(handle, EXT2_EXTENT_CURRENT,
					&extent))) {
		if (retval != EXT2_ET_NO_CURRENT_NODE)
			return retval;
		memset(&extent, 0, sizeof(extent));
	}
	if ((retval = ext2fs_extent_get_info(handle, &info)))
		return retval;
	orig_height = info.max_depth - info.curr_level;
	orig_lblk = extent.e_lblk;

	/* go to the logical spot we want to (re/un)map */
	retval = ext2fs_extent_goto(handle, logical);
	if (retval) {
		if (retval == EXT2_ET_EXTENT_NOT_FOUND) {
			retval = 0;
			mapped = 0;
			if (!physical) {
#ifdef DEBUG
				printf("block %llu already unmapped\n",
					logical);
#endif
				goto done;
			}
		} else
			goto done;
	}

	/*
	 * This may be the extent *before* the requested logical,
	 * if it's currently unmapped.
	 *
	 * Get the previous and next leaf extents, if they are present.
	 */
	retval = ext2fs_extent_get(handle, EXT2_EXTENT_CURRENT, &extent);
	if (retval)
		goto done;
	if (extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT)
		extent_uninit = 1;
	retval = ext2fs_extent_get(handle, EXT2_EXTENT_NEXT_LEAF, &next_extent);
	if (retval) {
		has_next = 0;
		if (retval != EXT2_ET_EXTENT_NO_NEXT)
			goto done;
	} else {
		dbg_print_extent("set_bmap: next_extent",
				 &next_extent);
		has_next = 1;
		if (next_extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT)
			next_uninit = 1;
	}
	retval = ext2fs_extent_goto(handle, logical);
	if (retval && retval != EXT2_ET_EXTENT_NOT_FOUND)
		goto done;
	retval = ext2fs_extent_get(handle, EXT2_EXTENT_PREV_LEAF, &prev_extent);
	if (retval) {
		has_prev = 0;
		if (retval != EXT2_ET_EXTENT_NO_PREV)
			goto done;
	} else {
		has_prev = 1;
		dbg_print_extent("set_bmap: prev_extent",
				 &prev_extent);
		if (prev_extent.e_flags & EXT2_EXTENT_FLAGS_UNINIT)
			prev_uninit = 1;
	}
	retval = ext2fs_extent_goto(handle, logical);
	if (retval && retval != EXT2_ET_EXTENT_NOT_FOUND)
		goto done;

	/* check if already pointing to the requested physical */
	if (mapped && (new_uninit == extent_uninit) &&
	    (extent.e_pblk + (logical - extent.e_lblk) == physical)) {
#ifdef DEBUG
		printf("physical block (at %llu) unchanged\n", logical);
#endif
		goto done;
	}

	if (!mapped) {
#ifdef DEBUG
		printf("mapping unmapped logical block %llu\n", logical);
#endif
		if ((logical == extent.e_lblk + extent.e_len) &&
		    (physical == extent.e_pblk + extent.e_len) &&
		    (new_uninit == extent_uninit) &&
		    ((int) extent.e_len < max_len-1)) {
			extent.e_len++;
			retval = ext2fs_extent_replace(handle, 0, &extent);
		} else if ((logical == extent.e_lblk - 1) &&
			   (physical == extent.e_pblk - 1) &&
			   (new_uninit == extent_uninit) &&
			   ((int) extent.e_len < max_len - 1)) {
			extent.e_len++;
			extent.e_lblk--;
			extent.e_pblk--;
			retval = ext2fs_extent_replace(handle, 0, &extent);
		} else if (has_next &&
			   (logical == next_extent.e_lblk - 1) &&
			   (physical == next_extent.e_pblk - 1) &&
			   (new_uninit == next_uninit) &&
			   ((int) next_extent.e_len < max_len - 1)) {
			retval = ext2fs_extent_get(handle,
						   EXT2_EXTENT_NEXT_LEAF,
						   &next_extent);
			if (retval)
				goto done;
			next_extent.e_len++;
			next_extent.e_lblk--;
			next_extent.e_pblk--;
			retval = ext2fs_extent_replace(handle, 0, &next_extent);
		} else if (logical < extent.e_lblk)
			retval = ext2fs_extent_insert(handle, 0, &newextent);
		else
			retval = ext2fs_extent_insert(handle,
				      EXT2_EXTENT_INSERT_AFTER, &newextent);
		if (retval)
			goto done;
		retval = ext2fs_extent_fix_parents(handle);
		if (retval)
			goto done;
	} else if ((logical == extent.e_lblk) && (extent.e_len == 1))  {
#ifdef DEBUG
		printf("(re/un)mapping only block in extent\n");
#endif
		if (physical) {
			retval = ext2fs_extent_replace(handle, 0, &newextent);
		} else {
			retval = ext2fs_extent_delete(handle, 0);
			if (retval)
				goto done;
			ec = ext2fs_extent_fix_parents(handle);
			if (ec != EXT2_ET_NO_CURRENT_NODE)
				retval = ec;
		}

		if (retval)
			goto done;
	} else if (logical == extent.e_lblk + extent.e_len - 1)  {
#ifdef DEBUG
		printf("(re/un)mapping last block in extent\n");
#endif
		if (physical) {
			if (has_next &&
			    (logical == (next_extent.e_lblk - 1)) &&
			    (physical == (next_extent.e_pblk - 1)) &&
			    (new_uninit == next_uninit) &&
			    ((int) next_extent.e_len < max_len - 1)) {
				retval = ext2fs_extent_get(handle,
					EXT2_EXTENT_NEXT_LEAF, &next_extent);
				if (retval)
					goto done;
				next_extent.e_len++;
				next_extent.e_lblk--;
				next_extent.e_pblk--;
				retval = ext2fs_extent_replace(handle, 0,
							       &next_extent);
				if (retval)
					goto done;
			} else
				retval = ext2fs_extent_insert(handle,
				      EXT2_EXTENT_INSERT_AFTER, &newextent);
			if (retval)
				goto done;
			/* Now pointing at inserted extent; move back to prev */
			retval = ext2fs_extent_get(handle,
						   EXT2_EXTENT_PREV_LEAF,
						   &extent);
			if (retval)
				goto done;
		}
		extent.e_len--;
		retval = ext2fs_extent_replace(handle, 0, &extent);
		if (retval)
			goto done;
	} else if (logical == extent.e_lblk) {
#ifdef DEBUG
		printf("(re/un)mapping first block in extent\n");
#endif
		if (physical) {
			if (has_prev &&
			    (logical == (prev_extent.e_lblk +
					 prev_extent.e_len)) &&
			    (physical == (prev_extent.e_pblk +
					  prev_extent.e_len)) &&
			    (new_uninit == prev_uninit) &&
			    ((int) prev_extent.e_len < max_len-1)) {
				retval = ext2fs_extent_get(handle, 
					EXT2_EXTENT_PREV_LEAF, &prev_extent);
				if (retval)
					goto done;
				prev_extent.e_len++;
				retval = ext2fs_extent_replace(handle, 0,
							       &prev_extent);
			} else
				retval = ext2fs_extent_insert(handle,
							      0, &newextent);
			if (retval)
				goto done;
			retval = ext2fs_extent_get(handle,
						   EXT2_EXTENT_NEXT_LEAF,
						   &extent);
			if (retval)
				goto done;
		}
		extent.e_pblk++;
		extent.e_lblk++;
		extent.e_len--;
		retval = ext2fs_extent_replace(handle, 0, &extent);
		if (retval)
			goto done;
	} else {
		__u32	orig_length;

#ifdef DEBUG
		printf("(re/un)mapping in middle of extent\n");
#endif
		/* need to split this extent; later */

		orig_length = extent.e_len;

		/* shorten pre-split extent */
		extent.e_len = (logical - extent.e_lblk);
		retval = ext2fs_extent_replace(handle, 0, &extent);
		if (retval)
			goto done;
		/* insert our new extent, if any */
		if (physical) {
			/* insert new extent after current */
			retval = ext2fs_extent_insert(handle,
					EXT2_EXTENT_INSERT_AFTER, &newextent);
			if (retval)
				goto done;
		}
		/* add post-split extent */
		extent.e_pblk += extent.e_len + 1;
		extent.e_lblk += extent.e_len + 1;
		extent.e_len = orig_length - extent.e_len - 1;
		retval = ext2fs_extent_insert(handle,
				EXT2_EXTENT_INSERT_AFTER, &extent);
		if (retval)
			goto done;
	}

done:
	/* get handle back to its position */
	if (orig_height > handle->max_depth)
		orig_height = handle->max_depth; /* In case we shortened the tree */
	extent_goto(handle, orig_height, orig_lblk);
	return retval;
}

errcode_t ext2fs_extent_delete(ext2_extent_handle_t handle, int flags)
{
	struct extent_path		*path;
	char 				*cp;
	struct ext3_extent_header	*eh;
	errcode_t			retval = 0;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	if (!(handle->fs->flags & EXT2_FLAG_RW))
		return EXT2_ET_RO_FILSYS;

	if (!handle->path)
		return EXT2_ET_NO_CURRENT_NODE;

#ifdef DEBUG
	{
		struct ext2fs_extent	extent;

		retval = ext2fs_extent_get(handle, EXT2_EXTENT_CURRENT,
					   &extent);
		if (retval == 0) {
			printf("extent delete %u ", handle->ino);
			dbg_print_extent(0, &extent);
		}
	}
#endif

	path = handle->path + handle->level;
	if (!path->curr)
		return EXT2_ET_NO_CURRENT_NODE;

	cp = path->curr;

	if (path->left) {
		memmove(cp, cp + sizeof(struct ext3_extent_idx),
			path->left * sizeof(struct ext3_extent_idx));
		path->left--;
	} else {
		struct ext3_extent_idx	*ix = path->curr;
		ix--;
		path->curr = ix;
	}
	if (--path->entries == 0)
		path->curr = 0;

	/* if non-root node has no entries left, remove it & parent ptr to it */
	if (path->entries == 0 && handle->level) {
		if (!(flags & EXT2_EXTENT_DELETE_KEEP_EMPTY)) {
			struct ext2fs_extent	extent;

			retval = ext2fs_extent_get(handle, EXT2_EXTENT_UP,
								&extent);
			if (retval)
				return retval;

			retval = ext2fs_extent_delete(handle, flags);
			handle->inode->i_blocks -= handle->fs->blocksize / 512;
			retval = ext2fs_write_inode(handle->fs, handle->ino,
						    handle->inode);
			ext2fs_block_alloc_stats(handle->fs, extent.e_pblk, -1);
		}
	} else {
		eh = (struct ext3_extent_header *) path->buf;
		eh->eh_entries = ext2fs_cpu_to_le16(path->entries);
		if ((path->entries == 0) && (handle->level == 0))
			eh->eh_depth = handle->max_depth = 0;
		retval = update_path(handle);
	}
	return retval;
}

errcode_t ext2fs_extent_get_info(ext2_extent_handle_t handle,
				 struct ext2_extent_info *info)
{
	struct extent_path		*path;

	EXT2_CHECK_MAGIC(handle, EXT2_ET_MAGIC_EXTENT_HANDLE);

	memset(info, 0, sizeof(struct ext2_extent_info));

	path = handle->path + handle->level;
	if (path) {
		if (path->curr)
			info->curr_entry = ((char *) path->curr - path->buf) /
				sizeof(struct ext3_extent_idx);
		else
			info->curr_entry = 0;
		info->num_entries = path->entries;
		info->max_entries = path->max_entries;
		info->bytes_avail = (path->max_entries - path->entries) *
			sizeof(struct ext3_extent);
	}

	info->curr_level = handle->level;
	info->max_depth = handle->max_depth;
	info->max_lblk = ((__u64) 1 << 32) - 1;
	info->max_pblk = ((__u64) 1 << 48) - 1;
	info->max_len = (1UL << 15);
	info->max_uninit_len = (1UL << 15) - 1;

	return 0;
}

#ifdef DEBUG

#include "ss/ss.h"

#include "debugfs.h"

/*
 * Hook in new commands into debugfs
 */
const char *debug_prog_name = "tst_extents";
extern ss_request_table extent_cmds;
ss_request_table *extra_cmds = &extent_cmds;

ext2_ino_t	current_ino = 0;
ext2_extent_handle_t current_handle;

int common_extent_args_process(int argc, char *argv[], int min_argc,
			       int max_argc, const char *cmd,
			       const char *usage, int flags)
{
	if (common_args_process(argc, argv, min_argc, max_argc, cmd,
				usage, flags))
		return 1;

	if (!current_handle) {
		com_err(cmd, 0, "Extent handle not open");
		return 1;
	}
	return 0;
}

void do_inode(int argc, char *argv[])
{
	ext2_ino_t	inode;
	int		i;
	struct ext3_extent_header *eh;
	errcode_t retval;

	if (check_fs_open(argv[0]))
		return;

	if (argc == 1) {
		if (current_ino)
			printf("Current inode is %d\n", current_ino);
		else
			printf("No current inode\n");
		return;
	}

	if (common_inode_args_process(argc, argv, &inode, 0)) {
		return;
	}

	current_ino = 0;

	retval = ext2fs_extent_open(current_fs, inode, &current_handle);
	if (retval) {
		com_err(argv[1], retval, "while opening extent handle");
		return;
	}

	current_ino = inode;

	printf("Loaded inode %d\n", current_ino);

	return;
}

void generic_goto_node(char *cmd_name, int op)
{
	struct ext2fs_extent	extent;
	errcode_t		retval;

	if (check_fs_open(cmd_name))
		return;

	if (!current_handle) {
		com_err(cmd_name, 0, "Extent handle not open");
		return;
	}

	retval = ext2fs_extent_get(current_handle, op, &extent);
	if (retval) {
		com_err(cmd_name, retval, 0);
		return;
	}
	dbg_print_extent(0, &extent);
}

void do_current_node(int argc, char *argv[])
{
	generic_goto_node(argv[0], EXT2_EXTENT_CURRENT);
}

void do_root_node(int argc, char *argv[])
{
	generic_goto_node(argv[0], EXT2_EXTENT_ROOT);
}

void do_last_leaf(int argc, char *argv[])
{
	generic_goto_node(argv[0], EXT2_EXTENT_LAST_LEAF);
}

void do_first_sib(int argc, char *argv[])
{
	generic_goto_node(argv[0], EXT2_EXTENT_FIRST_SIB);
}

void do_last_sib(int argc, char *argv[])
{
	generic_goto_node(argv[0], EXT2_EXTENT_LAST_SIB);
}

void do_next_sib(int argc, char *argv[])
{
	generic_goto_node(argv[0], EXT2_EXTENT_NEXT_SIB);
}

void do_prev_sib(int argc, char *argv[])
{
	generic_goto_node(argv[0], EXT2_EXTENT_PREV_SIB);
}

void do_next_leaf(int argc, char *argv[])
{
	generic_goto_node(argv[0], EXT2_EXTENT_NEXT_LEAF);
}

void do_prev_leaf(int argc, char *argv[])
{
	generic_goto_node(argv[0], EXT2_EXTENT_PREV_LEAF);
}

void do_next(int argc, char *argv[])
{
	generic_goto_node(argv[0], EXT2_EXTENT_NEXT);
}

void do_prev(int argc, char *argv[])
{
	generic_goto_node(argv[0], EXT2_EXTENT_PREV);
}

void do_up(int argc, char *argv[])
{
	generic_goto_node(argv[0], EXT2_EXTENT_UP);
}

void do_down(int argc, char *argv[])
{
	generic_goto_node(argv[0], EXT2_EXTENT_DOWN);
}

void do_delete_node(int argc, char *argv[])
{
	errcode_t	retval;
	int		err;

	if (common_extent_args_process(argc, argv, 1, 1, "delete_node",
				       "", CHECK_FS_RW | CHECK_FS_BITMAPS))
		return;

	retval = ext2fs_extent_delete(current_handle, 0);
	if (retval) {
		com_err(argv[0], retval, 0);
		return;
	}
	if (current_handle->path && current_handle->path[0].curr)
		do_current_node(argc, argv);
}

void do_replace_node(int argc, char *argv[])
{
	const char	*usage = "[--uninit] <lblk> <len> <pblk>";
	errcode_t	retval;
	struct ext2fs_extent extent;
	int err;

	if (common_extent_args_process(argc, argv, 3, 5, "replace_node",
				       usage, CHECK_FS_RW | CHECK_FS_BITMAPS))
		return;

	extent.e_flags = 0;

	if (!strcmp(argv[1], "--uninit")) {
		argc--;
		argv++;
		extent.e_flags |= EXT2_EXTENT_FLAGS_UNINIT;
	}

	if (argc != 4) {
		fprintf(stderr, "Usage: %s %s\n", argv[0], usage);
		return;
	}

	extent.e_lblk = parse_ulong(argv[1], argv[0], "logical block", &err);
	if (err)
		return;

	extent.e_len = parse_ulong(argv[2], argv[0], "logical block", &err);
	if (err)
		return;

	extent.e_pblk = parse_ulong(argv[3], argv[0], "logical block", &err);
	if (err)
		return;

	retval = ext2fs_extent_replace(current_handle, 0, &extent);
	if (retval) {
		com_err(argv[0], retval, 0);
		return;
	}
	do_current_node(argc, argv);
}

void do_split_node(int argc, char *argv[])
{
	errcode_t	retval;
	struct ext2fs_extent extent;
	int err;

	if (common_extent_args_process(argc, argv, 1, 1, "split_node",
				       "", CHECK_FS_RW | CHECK_FS_BITMAPS))
		return;

	retval = extent_node_split(current_handle);
	if (retval) {
		com_err(argv[0], retval, 0);
		return;
	}
	do_current_node(argc, argv);
}

void do_insert_node(int argc, char *argv[])
{
	const char	*usage = "[--after] [--uninit] <lblk> <len> <pblk>";
	errcode_t	retval;
	struct ext2fs_extent extent;
	char *cmd;
	int err;
	int flags = 0;

	if (common_extent_args_process(argc, argv, 3, 6, "insert_node",
				       usage, CHECK_FS_RW | CHECK_FS_BITMAPS))
		return;

	cmd = argv[0];

	extent.e_flags = 0;

	while (argc > 2) {
		if (!strcmp(argv[1], "--after")) {
			argc--;
			argv++;
			flags |= EXT2_EXTENT_INSERT_AFTER;
			continue;
		}
		if (!strcmp(argv[1], "--uninit")) {
			argc--;
			argv++;
			extent.e_flags |= EXT2_EXTENT_FLAGS_UNINIT;
			continue;
		}
		break;
	}

	if (argc != 4) {
		fprintf(stderr, "usage: %s %s\n", cmd, usage);
		return;
	}

	extent.e_lblk = parse_ulong(argv[1], cmd,
				    "logical block", &err);
	if (err)
		return;

	extent.e_len = parse_ulong(argv[2], cmd,
				    "length", &err);
	if (err)
		return;

	extent.e_pblk = parse_ulong(argv[3], cmd,
				    "pysical block", &err);
	if (err)
		return;

	retval = ext2fs_extent_insert(current_handle, flags, &extent);
	if (retval) {
		com_err(cmd, retval, 0);
		return;
	}
	do_current_node(argc, argv);
}

void do_set_bmap(int argc, char **argv)
{
	const char	*usage = "[--uninit] <lblk> <pblk>";
	errcode_t	retval;
	blk_t		logical;
	blk_t		physical;
	char		*cmd = argv[0];
	int		flags = 0;
	int		err;

	if (common_extent_args_process(argc, argv, 3, 5, "set_bmap",
				       usage, CHECK_FS_RW | CHECK_FS_BITMAPS))
		return;

	if (argc > 2 && !strcmp(argv[1], "--uninit")) {
		argc--;
		argv++;
		flags |= EXT2_EXTENT_SET_BMAP_UNINIT;
	}

	if (argc != 3) {
		fprintf(stderr, "Usage: %s %s\n", cmd, usage);
		return;
	}

	logical = parse_ulong(argv[1], cmd,
				    "logical block", &err);
	if (err)
		return;

	physical = parse_ulong(argv[2], cmd,
				    "physical block", &err);
	if (err)
		return;

	retval = ext2fs_extent_set_bmap(current_handle, logical,
					(blk64_t) physical, flags);
	if (retval) {
		com_err(cmd, retval, 0);
		return;
	}
	if (current_handle->path && current_handle->path[0].curr)
		do_current_node(argc, argv);
}

void do_print_all(int argc, char **argv)
{
	const char	*usage = "[--leaf-only|--reverse|--reverse-leaf]";
	struct ext2fs_extent	extent;
	errcode_t		retval;
	errcode_t		end_err = EXT2_ET_EXTENT_NO_NEXT;
	int			op = EXT2_EXTENT_NEXT;
	int			first_op = EXT2_EXTENT_ROOT;


	if (common_extent_args_process(argc, argv, 1, 2, "print_all",
				       usage, 0))
		return;

	if (argc == 2) {
		if (!strcmp(argv[1], "--leaf-only"))
			op = EXT2_EXTENT_NEXT_LEAF;
		else if (!strcmp(argv[1], "--reverse")) {
			op = EXT2_EXTENT_PREV;
			first_op = EXT2_EXTENT_LAST_LEAF;
			end_err = EXT2_ET_EXTENT_NO_PREV;
		} else if (!strcmp(argv[1], "--reverse-leaf")) {
			op = EXT2_EXTENT_PREV_LEAF;
			first_op = EXT2_EXTENT_LAST_LEAF;
			end_err = EXT2_ET_EXTENT_NO_PREV;
		} else {
			fprintf(stderr, "Usage: %s %s\n", argv[0], usage);
			return;
		}
	}

	retval = ext2fs_extent_get(current_handle, first_op, &extent);
	if (retval) {
		com_err(argv[0], retval, 0);
		return;
	}
	dbg_print_extent(0, &extent);

	while (1) {
		retval = ext2fs_extent_get(current_handle, op, &extent);
		if (retval == end_err)
			break;

		if (retval) {
			com_err(argv[0], retval, 0);
			return;
		}
		dbg_print_extent(0, &extent);
	}
}

void do_info(int argc, char **argv)
{
	struct ext2fs_extent	extent;
	struct ext2_extent_info	info;
	errcode_t		retval;

	if (common_extent_args_process(argc, argv, 1, 1, "info", "", 0))
		return;

	retval = ext2fs_extent_get_info(current_handle, &info);
	if (retval) {
		com_err(argv[0], retval, 0);
		return;
	}

	retval = ext2fs_extent_get(current_handle,
				   EXT2_EXTENT_CURRENT, &extent);
	if (retval) {
		com_err(argv[0], retval, 0);
		return;
	}

	dbg_print_extent(0, &extent);

	printf("Current handle location: %d/%d (max: %d, bytes %d), level %d/%d\n",
	       info.curr_entry, info.num_entries, info.max_entries,
	       info.bytes_avail, info.curr_level, info.max_depth);
	printf("\tmax lblk: %llu, max pblk: %llu\n", info.max_lblk,
	       info.max_pblk);
	printf("\tmax_len: %u, max_uninit_len: %u\n", info.max_len,
	       info.max_uninit_len);
}

void do_goto_block(int argc, char **argv)
{
	struct ext2fs_extent	extent;
	errcode_t		retval;
	int			op = EXT2_EXTENT_NEXT_LEAF;
	blk_t			blk;
	int			level = 0;

	if (common_extent_args_process(argc, argv, 2, 3, "goto_block",
				       "block [level]", 0))
		return;

	if (strtoblk(argv[0], argv[1], &blk))
		return;

	if (argc == 3)
		if (strtoblk(argv[0], argv[2], &level))
			return;

	retval = extent_goto(current_handle, level, (blk64_t) blk);

	if (retval) {
		com_err(argv[0], retval,
			"while trying to go to block %u, level %d",
			blk, level);
		return;
	}

	generic_goto_node(argv[0], EXT2_EXTENT_CURRENT);
}
#endif

