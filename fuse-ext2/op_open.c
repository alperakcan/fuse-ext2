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

ext2_file_t do_open (const char *path)
{
	errcode_t rc;
	ext2_ino_t ino;
	ext2_file_t efile;

	debugf("enter");
	debugf("path = %s", path);

	rc = ext2fs_namei(priv.fs, EXT2_ROOT_INO, EXT2_ROOT_INO, path, &ino);
	if (rc) {
		debugf("Error while trying to resolve %s", path);
		return NULL;
	}

	rc = ext2fs_file_open(priv.fs, ino, EXT2_FILE_WRITE, &efile);
	if (rc) {
		return NULL;
	}

	debugf("leave");
	return efile;
}

int op_open (const char *path, struct fuse_file_info *fi)
{
	ext2_file_t efile;

	debugf("enter");
	debugf("path = %s", path);

	efile = do_open(path);
	if (efile == NULL) {
		debugf("do_open(%s); failed", path);
		return -ENOENT;
	}
	fi->fh = (unsigned long) efile;

	debugf("leave");
	return 0;
}
