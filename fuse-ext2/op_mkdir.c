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

int op_mkdir (const char *path, mode_t mode)
{
	int rt;
	time_t tm;
	errcode_t rc;

	char *p_path;
	char *r_path;

	ext2_ino_t ino;
	struct ext2_inode inode;

	struct fuse_context *ctx;

	ext2_filsys e2fs = current_ext2fs();

	debugf("enter");
	debugf("path = %s, mode: 0%o, dir:0%o", path, mode, LINUX_S_IFDIR);

	rt=do_check_split(path, &p_path ,&r_path);
	if (rt != 0) {
		debugf("do_check(%s); failed", path);
		return rt;
	}

	debugf("parent: %s, child: %s, pathmax: %d", p_path, r_path, PATH_MAX);

	rt = do_readinode(e2fs, p_path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", p_path);
		free_split(p_path, r_path);
		return rt;
	}

	do {
		debugf("calling ext2fs_mkdir(e2fs, %d, 0, %s);", ino, r_path);
		rc = ext2fs_mkdir(e2fs, ino, 0, r_path);
		if (rc == EXT2_ET_DIR_NO_SPACE) {
			debugf("calling ext2fs_expand_dir(e2fs, &d)", ino);
			if (ext2fs_expand_dir(e2fs, ino)) {
				debugf("error while expanding directory %s (%d)", p_path, ino);
				free_split(p_path, r_path);
				return -ENOSPC;
			}
		}
	} while (rc == EXT2_ET_DIR_NO_SPACE);
	if (rc) {
		debugf("ext2fs_mkdir(e2fs, %d, 0, %s); failed (%d)", ino, r_path, rc);
		debugf("e2fs: %p, e2fs->inode_map: %p", e2fs, e2fs->inode_map);
		free_split(p_path, r_path);
		return -EIO;
	}

	rt = do_readinode(e2fs, path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", path);
		free_split(p_path, r_path);
		return -EIO;
	}
	tm = e2fs->now ? e2fs->now : time(NULL);
	inode.i_mode = LINUX_S_IFDIR | mode;
	inode.i_ctime = inode.i_atime = inode.i_mtime = tm;
	ctx = fuse_get_context();
	if (ctx) {
		inode.i_uid = ctx->uid;
		inode.i_gid = ctx->gid;
	}
	rc = ext2fs_write_inode(e2fs, ino, &inode);
	if (rc) {
		debugf("ext2fs_write_inode(e2fs, ino, &inode); failed");
		free_split(p_path, r_path);
		return -EIO;
	}

	/* update parent dir */
	rt = do_readinode(e2fs, p_path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); dailed", p_path);
		free_split(p_path, r_path);
		return -EIO;
	}
	inode.i_ctime = inode.i_mtime = tm;
	rc = ext2fs_write_inode(e2fs, ino, &inode);
	if (rc) {
		debugf("ext2fs_write_inode(e2fs, ino, &inode); failed");
		free_split(p_path, r_path);
		return -EIO;
	}

	free_split(p_path, r_path);

	debugf("leave");
	return 0;
}
