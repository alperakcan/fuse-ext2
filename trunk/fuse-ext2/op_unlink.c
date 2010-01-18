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

int op_unlink (const char *path)
{
	int rt;
	errcode_t rc;

	char *p_path;
	char *r_path;

	ext2_ino_t p_ino;
	struct ext2_inode p_inode;
	ext2_ino_t r_ino;
	struct ext2_vnode *r_vnode;
	struct ext2_inode *r_inode;

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
	rt = do_readvnode(e2fs, path, &r_ino, &r_vnode);
	if (rt) {
		debugf("do_readvnode(%s, &r_ino, &r_vnode); failed", path);
		free_split(p_path, r_path);
		return rt;

	}
	r_inode = vnode2inode(r_vnode);

	if(LINUX_S_ISDIR(r_inode->i_mode)) {
		debugf("%s is a directory", path);
		vnode_put(r_vnode,0);
		free_split(p_path, r_path);
		return -EISDIR;
	}

	rc = ext2fs_unlink(e2fs, p_ino, r_path, r_ino, 0);
	if (rc) {
		debugf("ext2fs_unlink(e2fs, %d, %s, %d, 0); failed", p_ino, r_path, r_ino);
		vnode_put(r_vnode,0);
		free_split(p_path, r_path);
		return -EIO;
	}

	if (r_inode->i_links_count > 0) {
		r_inode->i_links_count -= 1;
	}

	p_inode.i_ctime = p_inode.i_mtime = e2fs->now ? e2fs->now : time(NULL);
	if (rc) {
		debugf("ext2fs_write_inode(e2fs, p_ino, &p_inode); failed");
		vnode_put(r_vnode,1);
		free_split(p_path, r_path);
		return -EIO;
	}

	r_inode->i_ctime = e2fs->now ? e2fs->now : time(NULL);
	rc = vnode_put(r_vnode,1);
	if (rc) {
		debugf("vnode_put(r_vnode,1); failed");
		free_split(p_path, r_path);
		return -EIO;
	}

	free_split(p_path, r_path);
	debugf("leave");
	return 0;
}
