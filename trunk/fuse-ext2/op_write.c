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

int op_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int rt;
	size_t wr;
	const char *tmp;
	ext2_off_t fsize;
	unsigned int npos;
	ext2_file_t efile = EXT2FS_FILE(fi->fh);

	debugf("enter");
	debugf("path = %s", path);
	
	fsize = ext2fs_file_get_size(efile);
	if (offset + size > fsize) {
		rt = ext2fs_file_set_size(efile, offset + size);
		if (rt) {
			debugf("extfs_file_set_size(efile, %d); failed", offset + size);
			return rt;
		}
	}
	
	rt = ext2fs_file_lseek(efile, offset, SEEK_SET, &npos);
	if (rt) {
		debugf("ext2fs_file_lseek(efile, %d, SEEK_SET, &npos); failed", offset);
		return rt;
	}
	
	for (rt = 0, wr = 0, tmp = buf; size > 0 && rt == 0; size -= wr, tmp += wr) {
		debugf("size: %u, written: %u", size, wr);
		rt = ext2fs_file_write(efile, tmp, size, &wr);
	}
	if (rt) {
		debugf("ext2fs_file_write(edile, tmp, size, &wr); failed");
		return rt;
	}
	
	rt = ext2fs_file_flush(efile);
	if (rt) {
		debugf("ext2_file_flush(efile); failed");
		return rt;
	}

	debugf("leave");
	return wr;
}
