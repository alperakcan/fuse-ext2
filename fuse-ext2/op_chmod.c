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

int op_chmod (const char *path, mode_t mode)
{
	int rt;
	int mask;
	time_t tm;
	errcode_t rc;
	ext2_ino_t ino;
	struct ext2_inode inode;

	char *p_path;
	char *r_path;
	char *t_path;

	debugf("enter");
	debugf("path = %s 0%o", path, mode);

	p_path = strdup(path);
	if (p_path == NULL) {
		return -ENOMEM;
	}
	t_path = strrchr(p_path, '/');
	if (t_path == NULL) {
		debugf("this should not happen %s", p_path);
		free(p_path);
		return -ENOENT;
	}
	*t_path = '\0';
	r_path = t_path + 1;
	debugf("parent: %s, child: %s, pathmax: %d", p_path, r_path, PATH_MAX);

	if (strlen(r_path) > 255) {
		debugf("path exceeds 255 characters");
		free(p_path);
		return -ENAMETOOLONG;
	}
	free(p_path);

	rt = do_readinode(path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", path);
		return rt;
	}

	tm = priv.fs->now ? priv.fs->now : time(NULL);
	mask = S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX;
	inode.i_mode = (inode.i_mode & ~mask) | (mode & mask);
	inode.i_ctime = tm;

	rc = ext2fs_write_inode(priv.fs, ino, &inode);
	if (rc) {
		debugf("ext2fs_write_inode(priv.fs, ino, &inode); failed");
		return -EIO;
	}

	debugf("leave");
	return 0;
}
