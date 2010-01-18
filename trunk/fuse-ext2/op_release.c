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

static void release_callback (struct ext2_inode *inode, int flags)
{
	struct ext2_vnode *vnode = (struct ext2_vnode *) inode;
	vnode_put(vnode, (flags & EXT2_FILE_WRITE) != 0);
}

int do_release (ext2_file_t efile)
{
	errcode_t rc;

	debugf("enter");
	debugf("path = (%p)", efile);

	if (efile == NULL) {
		return -ENOENT;
	}
	rc = ext2fs_file_close2(efile, release_callback);
	if (rc) {
		return -EIO;
	}

	debugf("leave");
	return 0;
}

int op_release (const char *path, struct fuse_file_info *fi)
{
	int rt;
	ext2_file_t efile = (void *) (unsigned long) fi->fh;

	debugf("enter");
	debugf("path = %s (%p)", path, efile);
	rt = do_release(efile);
	if (rt != 0) {
		debugf("do_release() failed");
		return rt;
	}

	debugf("leave");
	return 0;
}
