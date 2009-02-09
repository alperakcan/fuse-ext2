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

struct dirbuf {
	char *p;
	fuse_fill_dir_t filler;
};

static int walk_dir (struct ext2_dir_entry *de, int offset, int blocksize, char *buf, void *priv_data)
{
	int ret;
	size_t flen;
	char *fname;
	struct dirbuf *b = priv_data;

	debugf("enter");
	
	flen = de->name_len & 0xff;
	fname = (char *) malloc(sizeof(char) * (flen + 1));
	if (fname == NULL) {
		debugf("s = (char *) malloc(sizeof(char) * (%d + 1)); failed", flen);
		return -ENOMEM;
	}
	snprintf(fname, flen + 1, "%s", de->name);
	debugf("b->filler(b->p, %s, NULL, 0);", fname);
	ret = b->filler(b->p, fname, NULL, 0);
	free(fname);
	
	debugf("leave");
	return ret;
}

int op_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	int rt;
	errcode_t rc;
	ext2_ino_t ino;
	struct ext2_inode inode;
	struct dirbuf b;

	debugf("enter");
	debugf("path = %s", path);
	
	rt = do_readinode(path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", path);
		return rt;
	}

	b.p = buf;
	b.filler = filler;
	
	rc = ext2fs_dir_iterate(priv.fs, ino, 0, NULL, walk_dir, &b);
	if (rc) {
		debugf("Error while trying to ext2fs_dir_iterate %s", path);
		return -EIO;
	}

	debugf("leave");
	return 0;
}
