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
	errcode_t rc;

	char *p_path;
	char *r_path;
	char *t_path;

	ext2_ino_t ino;
	struct ext2_inode inode;

	struct fuse_context *ctx;
	
	debugf("enter");
	debugf("path = %s, mode: 0%o", path, mode);

	p_path = strdup(path);
	if (p_path == NULL) {
		return -ENOMEM;
	}
	t_path = strrchr(p_path, '/');
	if (t_path == NULL) {
		debugf("this should not happen");
		free(p_path);
		return -ENOENT;
	}
	*t_path = '\0';
	r_path = t_path + 1;
	debugf("parent: %s, child: %s", p_path, r_path);
	
	rt = do_readinode(p_path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", p_path);
		free(p_path);
		return rt;
	}

	do {
		debugf("calling ext2fs_mkdir(priv.fs, %d, 0, %s);", ino, r_path);
		rc = ext2fs_mkdir(priv.fs, ino, 0, r_path);
		if (rc == EXT2_ET_DIR_NO_SPACE) {
			debugf("calling ext2fs_expand_dir(priv.fs, &d)", ino);
			if (ext2fs_expand_dir(priv.fs, ino)) {
				debugf("error while expanding directory %s (%d)", p_path, ino);
				free(p_path);
				return -ENOSPC;
			}
		}
	} while (rc == EXT2_ET_DIR_NO_SPACE);
	if (rc) {
		debugf("ext2fs_mkdir(priv.fs, %d, 0, %s); failed (%d)", ino, r_path, rc);
		debugf("priv.fs: %p, priv.fs->inode_map: %p", priv.fs, priv.fs->inode_map);
		free(p_path);
		return -EIO;
	}
	free(p_path);
	
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
