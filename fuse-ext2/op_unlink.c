/**
 * Copyright (c) 2008-2009 Alper Akcan <alper.akcan@gmail.com>
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

int op_unlink (const char *path)
{
	int rt;
	errcode_t rc;

	char *p_path;
	char *r_path;
	char *t_path;

	ext2_ino_t p_ino;
	struct ext2_inode p_inode;
	ext2_ino_t r_ino;
	struct ext2_inode r_inode;
	
	debugf("enter");
	debugf("path = %s", path);

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
	
	rt = do_readinode(p_path, &p_ino, &p_inode);
	if (rt) {
		debugf("do_readinode(%s, &p_ino, &p_inode); failed", p_path);
		free(p_path);
		return rt;
	}
	rt = do_readinode(path, &r_ino, &r_inode);
	if (rt) {
		debugf("do_readinode(%s, &r_ino, &r_inode); failed", path);
		free(p_path);
		return rt;
		
	}
	if(LINUX_S_ISDIR(r_inode.i_mode)) {
		debugf("%s is a directory", path);
		free(p_path);
		return -ENOTDIR;
	}

	if (r_inode.i_links_count > 0) {
		r_inode.i_links_count -= 1;
	}

	rc = ext2fs_unlink(priv.fs, p_ino, r_path, r_ino, 0);
	if (rc) {
		debugf("ext2fs_unlink(priv.fs, %d, %s, %d, 0); failed", p_ino, r_path, r_ino);
		free(p_path);
		return -EIO;
	}

	if (r_inode.i_links_count < 1) {
		rt = do_killfilebyinode(r_ino, &r_inode);
		if (rt) {
			debugf("do_killfilebyinode(r_ino, &r_inode); failed");
			free(p_path);
			return rt;
		}
	} else {
		rc = ext2fs_write_inode(priv.fs, r_ino, &r_inode);
		if (rc) {
			debugf("ext2fs_write_inode(priv.fs, r_ino, &r_inode); failed");
			free(p_path);
			return -EIO;
		}
	}
	
	free(p_path);
	debugf("leave");
	return 0;
}
