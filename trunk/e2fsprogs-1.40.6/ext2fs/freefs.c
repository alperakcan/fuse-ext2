/*
 * freefs.c --- free an ext2 filesystem
 *
 * Copyright (C) 1993, 1994, 1995, 1996 Theodore Ts'o.
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

#include "ext2_fs.h"
#include "ext2fsP.h"

static void ext2fs_free_inode_cache(struct ext2_inode_cache *icache);

void ext2fs_free(ext2_filsys fs)
{
	if (!fs || (fs->magic != EXT2_ET_MAGIC_EXT2FS_FILSYS))
		return;
	if (fs->image_io != fs->io) {
		if (fs->image_io)
			io_channel_close(fs->image_io);
	}
	if (fs->io) {
		io_channel_close(fs->io);
	}
	if (fs->device_name)
		ext2fs_free_mem(&fs->device_name);
	if (fs->super)
		ext2fs_free_mem(&fs->super);
	if (fs->orig_super)
		ext2fs_free_mem(&fs->orig_super);
	if (fs->group_desc)
		ext2fs_free_mem(&fs->group_desc);
	if (fs->block_map)
		ext2fs_free_block_bitmap(fs->block_map);
	if (fs->inode_map)
		ext2fs_free_inode_bitmap(fs->inode_map);

	if (fs->badblocks)
		ext2fs_badblocks_list_free(fs->badblocks);
	fs->badblocks = 0;

	if (fs->dblist)
		ext2fs_free_dblist(fs->dblist);

	if (fs->icache)
		ext2fs_free_inode_cache(fs->icache);

	fs->magic = 0;

	ext2fs_free_mem(&fs);
}

/*
 * Free the inode cache structure
 */
static void ext2fs_free_inode_cache(struct ext2_inode_cache *icache)
{
	if (--icache->refcount)
		return;
	if (icache->buffer)
		ext2fs_free_mem(&icache->buffer);
	if (icache->cache)
		ext2fs_free_mem(&icache->cache);
	icache->buffer_blk = 0;
	ext2fs_free_mem(&icache);
}

/*
 * This procedure frees a badblocks list.
 */
void ext2fs_u32_list_free(ext2_u32_list bb)
{
	if (bb->magic != EXT2_ET_MAGIC_BADBLOCKS_LIST)
		return;

	if (bb->list)
		ext2fs_free_mem(&bb->list);
	bb->list = 0;
	ext2fs_free_mem(&bb);
}

void ext2fs_badblocks_list_free(ext2_badblocks_list bb)
{
	ext2fs_u32_list_free((ext2_u32_list) bb);
}


/*
 * Free a directory block list
 */
void ext2fs_free_dblist(ext2_dblist dblist)
{
	if (!dblist || (dblist->magic != EXT2_ET_MAGIC_DBLIST))
		return;

	if (dblist->list)
		ext2fs_free_mem(&dblist->list);
	dblist->list = 0;
	if (dblist->fs && dblist->fs->dblist == dblist)
		dblist->fs->dblist = 0;
	dblist->magic = 0;
	ext2fs_free_mem(&dblist);
}

