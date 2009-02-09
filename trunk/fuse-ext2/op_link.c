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

int op_link (const char *source, const char *dest)
{
	int rc;
	char *p_path;
	char *r_path;
	char *t_path;

	ext2_ino_t ino;
	ext2_ino_t d_ino;
	struct ext2_inode inode;
	struct ext2_inode d_inode;

	debugf("source: %s, dest: %s", source, dest);

	rc = do_check(source);
	if (rc != 0) {
		debugf("do_check(%s); failed", source);
		return rc;
	}
	rc = do_check(dest);
	if (rc != 0) {
		debugf("do_check(%s); failed", dest);
		return rc;
	}

	rc = do_readinode(source, &ino, &inode);
	if (rc) {
		debugf("do_readinode(%s, &ino, &inode); failed", source);
		return rc;
	}

	p_path = strdup(dest);
	if (p_path == NULL) {
		debugf("strdup(%s); failed", dest);
		return -ENOMEM;
	}
	t_path = strrchr(p_path, '/');
	if (t_path == NULL) {
		debugf("this should not happen");
		free(p_path);
		return -ENOENT;
	}
	*t_path = '\0';
	r_path = t_path + 1;
	debugf("parent: %s, child: %s", p_path, r_path);

	rc = do_readinode(p_path, &d_ino, &d_inode);
	if (rc) {
		debugf("do_readinode(%s, &ino, &inode); failed", p_path);
		free(p_path);
		return rc;
	}

	do {
		debugf("calling ext2fs_link(priv.fs, %d, %s, %d, %d);", d_ino, r_path, ino, do_modetoext2lag(inode.i_mode));
		rc = ext2fs_link(priv.fs, d_ino, r_path, ino, do_modetoext2lag(inode.i_mode));
		if (rc == EXT2_ET_DIR_NO_SPACE) {
			debugf("calling ext2fs_expand_dir(priv.fs, &d)", ino);
			if (ext2fs_expand_dir(priv.fs, ino)) {
				debugf("error while expanding directory %s (%d)", p_path, ino);
				free(p_path);
				return -ENOSPC;
			}
		}
	} while (rc == EXT2_ET_DIR_NO_SPACE);
	if (rc) {
		debugf("ext2fs_link(priv.fs, %d, %s, %d, %d); failed", d_ino, r_path, ino, do_modetoext2lag(inode.i_mode));
		free(p_path);
		return -EIO;
	}

	d_inode.i_mtime = d_inode.i_ctime = inode.i_ctime = priv.fs->now ? priv.fs->now : time(NULL);
	rc = ext2fs_write_inode(priv.fs, d_ino, &d_inode);
	if (rc) {
		debugf("ext2fs_write_inode(priv.fs, d_ino, &d_inode); failed");
		free(p_path);
		return -EIO;
	}
	inode.i_links_count += 1;
	rc = ext2fs_write_inode(priv.fs, ino, &inode);
	if (rc) {
		debugf("ext2fs_write_inode(priv.fs, ino, &inode); failed");
		free(p_path);
		return -EIO;
	}
	debugf("done");

	return 0;
}
