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

int op_link (const char *source, const char *dest)
{
	int rc;
	char *p_path;
	char *r_path;

	ext2_ino_t s_ino;
	ext2_ino_t d_ino;
	struct ext2_inode s_inode;
	struct ext2_inode d_inode;
	ext2_filsys e2fs = current_ext2fs();

	debugf("source: %s, dest: %s", source, dest);

	rc = do_check(source);
	if (rc != 0) {
		debugf("do_check(%s); failed", source);
		return rc;
	}

	rc = do_check_split(dest, &p_path, &r_path);
	if (rc != 0) {
		debugf("do_check(%s); failed", dest);
		return rc;
	}

	debugf("parent: %s, child: %s", p_path, r_path);

	rc = do_readinode(e2fs, p_path, &d_ino, &d_inode);
	if (rc) {
		debugf("do_readinode(%s, &ino, &inode); failed", p_path);
		free_split(p_path, r_path);
		return rc;
	}

	rc = do_readinode(e2fs, source, &s_ino, &s_inode);
	if (rc) {
		debugf("do_readinode(%s, &s_ino, &s_inode); failed", p_path);
		free_split(p_path, r_path);
		return rc;
	}

	do {
		debugf("calling ext2fs_link(e2fs, %d, %s, %d, %d);", d_ino, r_path, s_ino, do_modetoext2lag(s_inode.i_mode));
		rc = ext2fs_link(e2fs, d_ino, r_path, s_ino, do_modetoext2lag(s_inode.i_mode));
		if (rc == EXT2_ET_DIR_NO_SPACE) {
			debugf("calling ext2fs_expand_dir(e2fs, &d)", d_ino);
			if (ext2fs_expand_dir(e2fs, d_ino)) {
				debugf("error while expanding directory %s (%d)", p_path, d_ino);
				free_split(p_path, r_path);
				return -ENOSPC;
			}
		}
	} while (rc == EXT2_ET_DIR_NO_SPACE);
	if (rc) {
		debugf("ext2fs_link(e2fs, %d, %s, %d, %d); failed", d_ino, r_path, s_ino, do_modetoext2lag(s_inode.i_mode));
		free_split(p_path, r_path);
		return -EIO;
	}

	d_inode.i_mtime = d_inode.i_ctime = s_inode.i_ctime = e2fs->now ? e2fs->now : time(NULL);
	s_inode.i_links_count += 1;

	rc = do_writeinode(e2fs, s_ino, &s_inode);
	if (rc) {
		debugf("do_writeinode(e2fs, s_ino, &s_inode); failed");
		free_split(p_path, r_path);
		return -EIO;
	}

	rc = do_writeinode(e2fs, d_ino, &d_inode);
	if (rc) {
		debugf("do_writeinode(e2fs, d_ino, &d_inode); failed");
		free_split(p_path, r_path);
		return -EIO;
	}

	free_split(p_path, r_path);
	debugf("done");

	return 0;
}
