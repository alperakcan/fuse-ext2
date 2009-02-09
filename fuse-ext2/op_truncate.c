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

int op_truncate(const char *path, off_t length)
{
	int rt;
	ext2_file_t efile;

	debugf("enter");
	debugf("path = %s", path);

	rt = do_check(path);
	if (rt != 0) {
		debugf("do_check(%s); failed", path);
		return rt;
	}
	efile = do_open(path);
	if (efile == NULL) {
		debugf("do_open(%s); failed", path);
		return -ENOENT;
	}

	rt = ext2fs_file_set_size(efile, length);
	if (rt) {
		do_release(efile);
		debugf("extfs_file_set_size(efile, %d); failed", length);
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
