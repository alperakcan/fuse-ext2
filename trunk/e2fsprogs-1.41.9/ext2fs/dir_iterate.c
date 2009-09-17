/*
 * dir_iterate.c --- ext2fs directory iteration operations
 *
 * Copyright (C) 1993, 1994, 1994, 1995, 1996, 1997 Theodore Ts'o.
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
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include "ext2_fs.h"
#include "ext2fsP.h"

#define EXT4_MAX_REC_LEN		((1<<16)-1)

errcode_t ext2fs_get_rec_len(ext2_filsys fs,
			     struct ext2_dir_entry *dirent,
			     unsigned int *rec_len)
{
	unsigned int len = dirent->rec_len;

	if (len == EXT4_MAX_REC_LEN || len == 0)
		*rec_len = fs->blocksize;
	else 
		*rec_len = (len & 65532) | ((len & 3) << 16);
	return 0;
}

errcode_t ext2fs_set_rec_len(ext2_filsys fs,
			     unsigned int len,
			     struct ext2_dir_entry *dirent)
{
	if ((len > fs->blocksize) || (fs->blocksize > (1 << 18)) || (len & 3))
		return EINVAL;
	if (len < 65536) {
		dirent->rec_len = len;
		return 0;
	}
	if (len == fs->blocksize) {
		if (fs->blocksize == 65536)
			dirent->rec_len = EXT4_MAX_REC_LEN;
		else 
			dirent->rec_len = 0;
	} else
		dirent->rec_len = (len & 65532) | ((len >> 16) & 3);
	return 0;
}

/*
 * This function checks to see whether or not a potential deleted
 * directory entry looks valid.  What we do is check the deleted entry
 * and each successive entry to make sure that they all look valid and
 * that the last deleted entry ends at the beginning of the next
 * undeleted entry.  Returns 1 if the deleted entry looks valid, zero
 * if not valid.
 */
static int ext2fs_validate_entry(ext2_filsys fs, char *buf,
				 unsigned int offset,
				 unsigned int final_offset)
{
	struct ext2_dir_entry *dirent;
	unsigned int rec_len;
#define DIRENT_MIN_LENGTH 12

	while ((offset < final_offset) &&
	       (offset <= fs->blocksize - DIRENT_MIN_LENGTH)) {
		dirent = (struct ext2_dir_entry *)(buf + offset);
		if (ext2fs_get_rec_len(fs, dirent, &rec_len))
			return 0;
		offset += rec_len;
		if ((rec_len < 8) ||
		    ((rec_len % 4) != 0) ||
		    ((((unsigned) dirent->name_len & 0xFF)+8) > rec_len))
			return 0;
	}
	return (offset == final_offset);
}

errcode_t ext2fs_dir_iterate2(ext2_filsys fs,
			      ext2_ino_t dir,
			      int flags,
			      char *block_buf,
			      int (*func)(ext2_ino_t	dir,
					  int		entry,
					  struct ext2_dir_entry *dirent,
					  int	offset,
					  int	blocksize,
					  char	*buf,
					  void	*priv_data),
			      void *priv_data)
{
	struct		dir_context	ctx;
	errcode_t	retval;

	EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

	retval = ext2fs_check_directory(fs, dir);
	if (retval)
		return retval;

	ctx.dir = dir;
	ctx.flags = flags;
	if (block_buf)
		ctx.buf = block_buf;
	else {
		retval = ext2fs_get_mem(fs->blocksize, &ctx.buf);
		if (retval)
			return retval;
	}
	ctx.func = func;
	ctx.priv_data = priv_data;
	ctx.errcode = 0;
	retval = ext2fs_block_iterate2(fs, dir, BLOCK_FLAG_READ_ONLY, 0,
				       ext2fs_process_dir_block, &ctx);
	if (!block_buf)
		ext2fs_free_mem(&ctx.buf);
	if (retval)
		return retval;
	return ctx.errcode;
}

struct xlate {
	int (*func)(struct ext2_dir_entry *dirent,
		    int		offset,
		    int		blocksize,
		    char	*buf,
		    void	*priv_data);
	void *real_private;
};

static int xlate_func(ext2_ino_t dir EXT2FS_ATTR((unused)),
		      int entry EXT2FS_ATTR((unused)),
		      struct ext2_dir_entry *dirent, int offset,
		      int blocksize, char *buf, void *priv_data)
{
	struct xlate *xl = (struct xlate *) priv_data;

	return (*xl->func)(dirent, offset, blocksize, buf, xl->real_private);
}

extern errcode_t ext2fs_dir_iterate(ext2_filsys fs,
			      ext2_ino_t dir,
			      int flags,
			      char *block_buf,
			      int (*func)(struct ext2_dir_entry *dirent,
					  int	offset,
					  int	blocksize,
					  char	*buf,
					  void	*priv_data),
			      void *priv_data)
{
	struct xlate xl;

	xl.real_private = priv_data;
	xl.func = func;

	return ext2fs_dir_iterate2(fs, dir, flags, block_buf,
				   xlate_func, &xl);
}


/*
 * Helper function which is private to this module.  Used by
 * ext2fs_dir_iterate() and ext2fs_dblist_dir_iterate()
 */
int ext2fs_process_dir_block(ext2_filsys fs,
			     blk_t	*blocknr,
			     e2_blkcnt_t blockcnt,
			     blk_t	ref_block EXT2FS_ATTR((unused)),
			     int	ref_offset EXT2FS_ATTR((unused)),
			     void	*priv_data)
{
	struct dir_context *ctx = (struct dir_context *) priv_data;
	unsigned int	offset = 0;
	unsigned int	next_real_entry = 0;
	int		ret = 0;
	int		changed = 0;
	int		do_abort = 0;
	unsigned int	rec_len, size;
	int		entry;
	struct ext2_dir_entry *dirent;

	if (blockcnt < 0)
		return 0;

	entry = blockcnt ? DIRENT_OTHER_FILE : DIRENT_DOT_FILE;

	ctx->errcode = ext2fs_read_dir_block(fs, *blocknr, ctx->buf);
	if (ctx->errcode)
		return BLOCK_ABORT;

	while (offset < fs->blocksize) {
		dirent = (struct ext2_dir_entry *) (ctx->buf + offset);
		if (ext2fs_get_rec_len(fs, dirent, &rec_len))
			return BLOCK_ABORT;
		if (((offset + rec_len) > fs->blocksize) ||
		    (rec_len < 8) ||
		    ((rec_len % 4) != 0) ||
		    ((((unsigned) dirent->name_len & 0xFF)+8) > rec_len)) {
			ctx->errcode = EXT2_ET_DIR_CORRUPTED;
			return BLOCK_ABORT;
		}
		if (!dirent->inode &&
		    !(ctx->flags & DIRENT_FLAG_INCLUDE_EMPTY))
			goto next;

		ret = (ctx->func)(ctx->dir,
				  (next_real_entry > offset) ?
				  DIRENT_DELETED_FILE : entry,
				  dirent, offset,
				  fs->blocksize, ctx->buf,
				  ctx->priv_data);
		if (entry < DIRENT_OTHER_FILE)
			entry++;

		if (ret & DIRENT_CHANGED) {
			if (ext2fs_get_rec_len(fs, dirent, &rec_len))
				return BLOCK_ABORT;
			changed++;
		}
		if (ret & DIRENT_ABORT) {
			do_abort++;
			break;
		}
next:
 		if (next_real_entry == offset)
			next_real_entry += rec_len;

 		if (ctx->flags & DIRENT_FLAG_INCLUDE_REMOVED) {
			size = ((dirent->name_len & 0xFF) + 11) & ~3;

			if (rec_len != size)  {
				unsigned int final_offset;

				final_offset = offset + rec_len;
				offset += size;
				while (offset < final_offset &&
				       !ext2fs_validate_entry(fs, ctx->buf,
							      offset,
							      final_offset))
					offset += 4;
				continue;
			}
		}
		offset += rec_len;
	}

	if (changed) {
		ctx->errcode = ext2fs_write_dir_block(fs, *blocknr, ctx->buf);
		if (ctx->errcode)
			return BLOCK_ABORT;
	}
	if (do_abort)
		return BLOCK_ABORT;
	return 0;
}

