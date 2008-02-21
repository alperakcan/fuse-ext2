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

int op_mkdir (const char *path, mode_t mode)
{
	int rt;
	char *tmp;
	char *path_parent;
	char *path_real;

	errcode_t rc;
	ext2_ino_t ino;
	struct ext2_inode inode;

	struct fuse_context *ctx;
	
	debugf("enter");
	debugf("path = %s, mode: 0%o", path, mode);

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
	
	rt = do_readinode(path_parent, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", path_parent);
		free(path_parent);
		return rt;
	}

	do {
		debugf("calling ext2fs_mkdir(priv.fs, %d, 0, %s);", ino, path_real);
		rc = ext2fs_mkdir(priv.fs, ino, 0, path_real);
		if (rc == EXT2_ET_DIR_NO_SPACE) {
			debugf("calling ext2fs_expand_dir(priv.fs, &d)", ino);
			if (ext2fs_expand_dir(priv.fs, ino)) {
				debugf("error while expanding directory %s (%d)", path_parent, ino);
				free(path_parent);
				return -ENOSPC;
			}
		}
	} while (rc == EXT2_ET_DIR_NO_SPACE);
	if (rc) {
		debugf("ext2fs_mkdir(priv.fs, %d, 0, %s); failed (%d)", ino, path_real, rc);
		debugf("priv.fs: %p, priv.fs->inode_map: %p", priv.fs, priv.fs->inode_map);
		free(path_parent);
		return -EIO;
	}
	free(path_parent);
	
	rt = do_readinode(path, &ino, &inode);
	if (rt) {
		debugf("de_readinode(%s, &ino, &inode); failed", path);
		return -EIO;
	}
	ctx = fuse_get_context();
	if (ctx) {
		inode.i_uid = ctx->uid;
		inode.i_gid = ctx->gid;
		rc = ext2fs_write_inode(priv.fs, ino, &inode);
		if (rc) {
			debugf("ext2fs_write_inode(priv.fs, ino, &inode); failed");
			return -EIO;
		}
	}

	debugf("leave");
	return 0;
}
