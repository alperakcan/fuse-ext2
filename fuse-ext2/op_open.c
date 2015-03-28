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

ext2_file_t do_open (ext2_filsys e2fs, const char *path, int flags)
{
	errcode_t rc;
	ext2_ino_t ino;
	ext2_file_t efile;
	struct ext2_vnode *vnode;
	int rt;

	debugf("enter");
	debugf("path = %s", path);

	rt = do_readvnode(e2fs, path, &ino, &vnode);
	if (rt) {
		debugf("do_readvnode(%s, &ino, &vnode); failed", path);
		return NULL;
	}

	rc = ext2fs_file_open2(e2fs, ino, vnode2inode(vnode),
			(((flags & O_ACCMODE) != 0) ? EXT2_FILE_WRITE : 0) | EXT2_FILE_SHARED_INODE, 
			&efile);

	if (rc) {
		vnode_put(vnode,0);
		return NULL;
	}

	debugf("efile: %p", efile);
	debugf("leave");
	return efile;
}

int op_open (const char *path, struct fuse_file_info *fi)
{
	ext2_file_t efile;
	ext2_filsys e2fs = current_ext2fs();

	debugf("enter");
	debugf("path = %s", path);

	efile = do_open(e2fs, path, fi->flags);
	if (efile == NULL) {
		debugf("do_open(%s); failed", path);
		return -ENOENT;
	}
	fi->fh = (uint64_t) efile;

	debugf("leave");
	return 0;
}
