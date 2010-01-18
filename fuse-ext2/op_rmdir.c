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

struct rmdir_st {
	ext2_ino_t parent;
	int empty;
};

static int rmdir_proc (ext2_ino_t dir EXT2FS_ATTR((unused)),
		       int entry EXT2FS_ATTR((unused)),
		       struct ext2_dir_entry *dirent,
		       int offset EXT2FS_ATTR((unused)),
		       int blocksize EXT2FS_ATTR((unused)),
		       char *buf EXT2FS_ATTR((unused)), void *private)
{
	int *p_empty= (int *) private;

	debugf("enter");
	debugf("walking on: %s", dirent->name);

	if (dirent->inode == 0 ||
			(((dirent->name_len & 0xFF) == 1) && (dirent->name[0] == '.')) ||
			(((dirent->name_len & 0xFF) == 2) && (dirent->name[0] == '.') && 
			 (dirent->name[1] == '.'))) {
		debugf("leave");
		return 0;
	}
	*p_empty = 0;
	debugf("leave (not empty)");
	return 0;
}

int do_check_empty_dir(ext2_filsys e2fs, ext2_ino_t ino)
{
	errcode_t rc;
	int empty = 1;

	rc = ext2fs_dir_iterate2(e2fs, ino, 0, 0, rmdir_proc, &empty);
	if (rc) {
		debugf("while iterating over directory");
		return -EIO;
	}

	if (empty == 0) {
		debugf("directory not empty");
		return -ENOTEMPTY;
	}

	return 0;
}

int op_rmdir (const char *path)
{
	int rt;
	errcode_t rc;

	char *p_path;
	char *r_path;

	ext2_ino_t p_ino;
	struct ext2_inode p_inode;
	ext2_ino_t r_ino;
	struct ext2_inode r_inode;
	
	ext2_filsys e2fs = current_ext2fs();

	debugf("enter");
	debugf("path = %s", path);

	rt=do_check_split(path, &p_path, &r_path);
	if (rt != 0) {
		debugf("do_check_split: failed");
		return rt;
	}

	debugf("parent: %s, child: %s", p_path, r_path);
	
	rt = do_readinode(e2fs, p_path, &p_ino, &p_inode);
	if (rt) {
		debugf("do_readinode(%s, &p_ino, &p_inode); failed", p_path);
		free_split(p_path, r_path);
		return rt;
	}
	rt = do_readinode(e2fs, path, &r_ino, &r_inode);
	if (rt) {
		debugf("do_readinode(%s, &r_ino, &r_inode); failed", path);
		free_split(p_path, r_path);
		return rt;
		
	}
	if (!LINUX_S_ISDIR(r_inode.i_mode)) {
		debugf("%s is not a directory", path);
		free_split(p_path, r_path);
		return -ENOTDIR;
	}
	if (r_ino == EXT2_ROOT_INO) {
		debugf("root dir cannot be removed", path);
		free_split(p_path, r_path);
		return -EIO;
	}
	
	rt = do_check_empty_dir(e2fs, r_ino);
	if (rt) {
		debugf("do_check_empty_dir filed");
		free_split(p_path, r_path);
		return rt;
	}

	rc = ext2fs_unlink(e2fs, p_ino, r_path, r_ino, 0);
	if (rc) {
		debugf("while unlinking ino %d", (int) r_ino);
		free_split(p_path, r_path);
		return -EIO;
	}

	rt = do_killfilebyinode(e2fs, r_ino, &r_inode);
	if (rt) {
		debugf("do_killfilebyinode(r_ino, &r_inode); failed");
		free_split(p_path, r_path);
		return rt;
	}

	rt = do_readinode(e2fs, p_path, &p_ino, &p_inode);
	if (rt) {
		debugf("do_readinode(p_path, &p_ino, &p_inode); failed");
		free_split(p_path, r_path);
		return rt;
	}
	if (p_inode.i_links_count > 1) {
		p_inode.i_links_count--;
	}
	rc = ext2fs_write_inode(e2fs, p_ino, &p_inode);
	if (rc) {
		debugf("ext2fs_write_inode(e2fs, ino, inode); failed");
		free_split(p_path, r_path);
		return -EIO;
	}

	free_split(p_path, r_path);

	debugf("leave");
	return 0;
}
