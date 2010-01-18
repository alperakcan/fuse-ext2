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

int op_truncate(const char *path, off_t length)
{
	int rt;
	ext2_file_t efile;
	ext2_filsys e2fs = current_ext2fs();

	debugf("enter");
	debugf("path = %s", path);

	rt = do_check(path);
	if (rt != 0) {
		debugf("do_check(%s); failed", path);
		return rt;
	}
	efile = do_open(e2fs, path, O_WRONLY);
	if (efile == NULL) {
		debugf("do_open(%s); failed", path);
		return -ENOENT;
	}

	rt = ext2fs_file_set_size(efile, length);
	if (rt) {
		do_release(efile);
		debugf("ext2fs_file_set_size(efile, %d); failed", length);
		return rt;
	}

	rt = do_release(efile);
	if (rt != 0) {
		debugf("do_release(efile); failed");
		return rt;
	}

	debugf("leave");
	return 0;
}

int op_ftruncate(const char *path, off_t length, struct fuse_file_info *fi)
{
	size_t rt;
	ext2_file_t efile = EXT2FS_FILE(fi->fh);

	debugf("enter");
	debugf("path = %s", path);

	rt = ext2fs_file_set_size(efile, length);
	if (rt) {
		debugf("ext2fs_file_set_size(efile, %d); failed", length);
		return rt;
	}

	debugf("leave");
	  return 0;
}
