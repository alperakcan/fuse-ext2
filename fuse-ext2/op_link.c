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

int op_link (const char *source, const char *dest)
{
	int rc;
	char *p_path;
	char *r_path;

	ext2_ino_t ino;
	ext2_ino_t d_ino;
	struct ext2_vnode *vnode;
	struct ext2_inode *inode;
	struct ext2_inode d_inode;
	ext2_filsys e2fs = current_ext2fs();

	debugf("source: %s, dest: %s", source, dest);

	rc = do_check(source);
	if (rc != 0) {
		debugf("do_check(%s); failed", source);
		return rc;
	}

	rc = do_check_split(dest, &p_path, &r_path);
	if (rc != 0) {
		debugf("do_check(%s); failed", dest);
		return rc;
	}

	debugf("parent: %s, child: %s", p_path, r_path);

	rc = do_readinode(e2fs, p_path, &d_ino, &d_inode);
	if (rc) {
		debugf("do_readinode(%s, &ino, &inode); failed", p_path);
		free_split(p_path, r_path);
		return rc;
	}

	rc = do_readvnode(e2fs, source, &ino, &vnode);
	if (rc) {
		debugf("do_readvnode(%s, &ino, &vnode); failed", source);
		free_split(p_path, r_path);
		return rc;
	}

	inode = vnode2inode(vnode);

	do {
		debugf("calling ext2fs_link(e2fs, %d, %s, %d, %d);", d_ino, r_path, ino, do_modetoext2lag(inode->i_mode));
		rc = ext2fs_link(e2fs, d_ino, r_path, ino, do_modetoext2lag(inode->i_mode));
		if (rc == EXT2_ET_DIR_NO_SPACE) {
			debugf("calling ext2fs_expand_dir(e2fs, &d)", d_ino);
			if (ext2fs_expand_dir(e2fs, d_ino)) {
				debugf("error while expanding directory %s (%d)", p_path, d_ino);
				vnode_put(vnode, 0);
				free_split(p_path, r_path);
				return -ENOSPC;
			}
		}
	} while (rc == EXT2_ET_DIR_NO_SPACE);
	if (rc) {
		debugf("ext2fs_link(e2fs, %d, %s, %d, %d); failed", d_ino, r_path, ino, do_modetoext2lag(inode->i_mode));
		vnode_put(vnode, 0);
		free_split(p_path, r_path);
		return -EIO;
	}

	d_inode.i_mtime = d_inode.i_ctime = inode->i_ctime = e2fs->now ? e2fs->now : time(NULL);
	inode->i_links_count += 1;
	rc=vnode_put(vnode,1);
	if (rc) {
		debugf("vnode_put(vnode,1); failed");
		free_split(p_path, r_path);
		return -EIO;
	}
	rc = ext2fs_write_inode(e2fs, d_ino, &d_inode);
	if (rc) {
		debugf("ext2fs_write_inode(e2fs, d_ino, &d_inode); failed");
		free_split(p_path, r_path);
		return -EIO;
	}
	free_split(p_path, r_path);
	debugf("done");

	return 0;
}
