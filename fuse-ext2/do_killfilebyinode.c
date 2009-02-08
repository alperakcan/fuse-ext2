
#include "fuse-ext2.h"

static int release_blocks_proc (ext2_filsys fs, blk_t *blocknr, int blockcnt, void *private)
{
	blk_t block;

	debugf("enter");

	block = *blocknr;
	ext2fs_block_alloc_stats(fs, block, -1);

	debugf("leave");
	return 0;
}

int do_killfilebyinode (ext2_ino_t ino, struct ext2_inode *inode)
{
	errcode_t rc;

	debugf("enter");

	inode->i_links_count = 0;
	inode->i_dtime = time(NULL);

	rc = ext2fs_write_inode(priv.fs, ino, inode);
	if (rc) {
		debugf("ext2fs_write_inode(priv.fs, ino, inode); failed");
		return -EIO;
	}

	if (!ext2fs_inode_has_valid_blocks(inode)) {
		debugf("inode has no blocks, leaving");
		return 0;
	}

	debugf("start block delete for %d", ino);

	ext2fs_block_iterate(priv.fs, ino, 0, NULL, release_blocks_proc, NULL);
	ext2fs_inode_alloc_stats2(priv.fs, ino, -1, LINUX_S_ISDIR(inode->i_mode));

	debugf("leave");
	return 0;
}
