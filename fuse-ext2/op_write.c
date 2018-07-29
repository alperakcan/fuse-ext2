/**
 * Copyright (c) 2008-2015 Alper Akcan <alper.akcan@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "fuse-ext2.h"

size_t do_write (ext2_file_t efile, const char *buf, size_t size, off_t offset)
{
	int rt;
	const char *tmp;
	unsigned int wr;
	unsigned long long npos;
	unsigned long long fsize;
	unsigned long long wsize;

	debugf("enter");

	rt = ext2fs_file_get_lsize(efile, &fsize);
	if (rt != 0) {
		debugf("ext2fs_file_get_lsize(efile, &fsize); failed");
		return -EIO;
	}

	rt = ext2fs_file_llseek(efile, offset, SEEK_SET, &npos);
	if (rt) {
		debugf("ext2fs_file_lseek(efile, %lld, SEEK_SET, &npos); failed", offset);
		return rt;
	}

	for (rt = 0, wr = 0, tmp = buf, wsize = 0; size > 0 && rt == 0; size -= wr, wsize += wr, tmp += wr) {
		rt = ext2fs_file_write(efile, tmp, size, &wr);
		debugf("rt: %d, size: %u, written: %u", rt, size, wr);
	}
	if (rt != 0 && rt != EXT2_ET_BLOCK_ALLOC_FAIL) {
		debugf("ext2fs_file_write(edile, tmp, size, &wr); failed");
		return -EIO;
	}

	if (offset + wsize > fsize) {
		rt = ext2fs_file_set_size2(efile, offset + wsize);
		if (rt) {
			debugf("extfs_file_set_size(efile, %lld); failed", offset + size);
			return -EIO;
		}
	}

	rt = ext2fs_file_flush(efile);
	if (rt != 0 && rt != EXT2_ET_BLOCK_ALLOC_FAIL) {
		debugf("ext2_file_flush(efile); failed");
		return -EIO;
	}

	debugf("leave");
	return wsize;
}

int op_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	size_t rt;
	ext2_file_t efile = EXT2FS_FILE(fi->fh);
	ext2_filsys e2fs = current_ext2fs();

	debugf("enter");
	debugf("path = %s", path);

	efile = do_open(e2fs, path, O_WRONLY);
	rt = do_write(efile, buf, size, offset);
	do_release(efile);

	debugf("leave");
	return rt;
}
