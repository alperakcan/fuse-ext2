/**
 * Copyright (c) 2008 Alper Akcan
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

struct rmdir_st {
	ext2_ino_t parent;
	int empty;
};

static int release_blocks_proc (ext2_filsys fs, blk_t *blocknr, int blockcnt EXT2FS_ATTR((unused)), void *private EXT2FS_ATTR((unused)))
{
	blk_t block;
	
	debugf("enter");

	block = *blocknr;
	ext2fs_block_alloc_stats(fs, block, -1);
	
	debugf("leave");
	return 0;
}

static int kill_file_by_inode (ext2_ino_t ino, struct ext2_inode *inode)
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
		debugf("ext2fs_inode_has_valid_blocks(inode); failed");
		return -EIO;
	}

	debugf("start block delete for %d", ino);

	ext2fs_block_iterate(priv.fs, ino, 0, NULL, release_blocks_proc, NULL);
	ext2fs_inode_alloc_stats2(priv.fs, ino, -1, LINUX_S_ISDIR(inode->i_mode));
	
	debugf("leave");
	return 0;
}

static int rmdir_proc (ext2_ino_t dir EXT2FS_ATTR((unused)),
		       int entry EXT2FS_ATTR((unused)),
		       struct ext2_dir_entry *dirent,
		       int offset EXT2FS_ATTR((unused)),
		       int blocksize EXT2FS_ATTR((unused)),
		       char *buf EXT2FS_ATTR((unused)), void *private)
{
	struct rmdir_st *rds = (struct rmdir_st *) private;

	debugf("enter");
	debugf("walking on: %s", dirent->name);

	if (dirent->inode == 0) {
		debugf("leave");
		return 0;
	}
	if (((dirent->name_len & 0xFF) == 1) && (dirent->name[0] == '.')) {
		debugf("leave");
		return 0;
	}
	if (((dirent->name_len & 0xFF) == 2) && (dirent->name[0] == '.') && (dirent->name[1] == '.')) {
		rds->parent = dirent->inode;
		debugf("leave");
		return 0;
	}
	rds->empty = 0;
	debugf("leave");
	return 0;
}

int op_rmdir (const char *path)
{
	int rt;
	char *tmp;
	char *path_parent;
	char *path_real;

	errcode_t rc;
	ext2_ino_t p_ino;
	struct ext2_inode p_inode;
	ext2_ino_t r_ino;
	struct ext2_inode r_inode;
	
	struct rmdir_st rds;

	debugf("enter");
	debugf("path = %s", path);

	path_parent = strdup(path);
	if (path_parent == NULL) {
		return -ENOMEM;
	}
	tmp = strrchr(path_parent, '/');
	if (tmp == NULL) {
		debugf("this should not happen");
		free(path_parent);
		return -ENOENT;
	}
	*tmp = '\0';
	path_real = tmp + 1;
	debugf("parent: %s, child: %s", path_parent, path_real);
	
	rt = do_readinode(path_parent, &p_ino, &p_inode);
	if (rt) {
		debugf("do_readinode(%s, &p_ino, &p_inode); failed", path_parent);
		free(path_parent);
		return rt;
	}
	rt = do_readinode(path, &r_ino, &r_inode);
	if (rt) {
		debugf("do_readinode(%s, &r_ino, &r_inode); failed", path);
		free(path_parent);
		return rt;
		
	}
	if(!LINUX_S_ISDIR(r_inode.i_mode)) {
		debugf("%s is not a directory", path);
		free(path_parent);
		return -ENOTDIR;
	}
	
	rds.parent = 0;
	rds.empty = 1;

	rc = ext2fs_dir_iterate2(priv.fs, r_ino, 0, 0, rmdir_proc, &rds);
	if (rc) {
		debugf("while iterating over directory");
		free(path_parent);
		return -EIO;
	}

	if (rds.empty == 0) {
		debugf("directory not empty");
		free(path_parent);
		return -ENOTEMPTY;
	}

	rc = ext2fs_unlink(priv.fs, p_ino, path_real, r_ino, 0);
	if (rc) {
		debugf("while unlinking ino %d", (int) r_ino);
		free(path_parent);
		return -EIO;
	}

	rt = kill_file_by_inode(r_ino, &r_inode);
	if (rt) {
		debugf("kill_file_by_inode(r_ino, &r_inode); failed");
		free(path_parent);
		return rt;
	}

	if (rds.parent) {
		rt = do_readinode(path_parent, &p_ino, &p_inode);
		if (rt) {
			debugf("do_readinode(path_parent, &p_ino, &p_inode); failed");
			free(path_parent);
			return rt;
		}
		if (p_inode.i_links_count > 1) {
			p_inode.i_links_count--;
		}
		rc = ext2fs_write_inode(priv.fs, p_ino, &p_inode);
		if (rc) {
			debugf("ext2fs_write_inode(priv.fs, ino, inode); failed");
			free(path_parent);
			return -EIO;
		}
	} else {
		debugf("this should not happen, deleted dir has no parent");
		return -EIO;
	}

	free(path_parent);
	
	debugf("leave");
	return 0;
}
