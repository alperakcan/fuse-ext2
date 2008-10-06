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

int op_symlink (const char *sourcename, const char *destname)
{
	int rt;
	size_t wr;
	ext2_file_t efile;

	debugf("enter");
	debugf("source: %s, dest: %s", sourcename, destname);

	rt = do_create(destname, LINUX_S_IFLNK | 0777);
	if (rt != 0) {
		debugf("do_create(%s, LINUX_S_IFLNK | 0777); failed", destname);
		return rt;
	}
	efile = do_open(destname);
	if (efile == NULL) {
		debugf("do_open(%s); failed", destname);
		return -EIO;
	}
	wr = do_write(efile, sourcename, strlen(sourcename) + 1, 0);
	if (wr != (strlen(sourcename) + 1)) {
		debugf("do_write(efile, %s, %d, 0); failed", sourcename, strlen(sourcename) + 1);
		return -EIO;
	}
	rt = do_release(efile);
	if (rt != 0) {
		debugf("do_release(efile); failed");
		return rt;
	}
	debugf("leave");
	return 0;
}
