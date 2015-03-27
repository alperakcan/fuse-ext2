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

int do_modetoext2lag (mode_t mode)
{
	if (S_ISREG(mode)) {
		return EXT2_FT_REG_FILE;
	} else if (S_ISDIR(mode)) {
		return EXT2_FT_DIR;
	} else if (S_ISCHR(mode)) {
		return EXT2_FT_CHRDEV;
	} else if (S_ISBLK(mode)) {
		return EXT2_FT_BLKDEV;
	} else if (S_ISFIFO(mode)) {
		return EXT2_FT_FIFO;
	} else if (S_ISSOCK(mode)) {
		return EXT2_FT_SOCK;
	} else if (S_ISLNK(mode)) {
		return EXT2_FT_SYMLINK;
	}
	return EXT2_FT_UNKNOWN;
}

static inline int old_valid_dev(dev_t dev)
{
	return major(dev) < 256 && minor(dev) < 256;
}

static inline __u16 old_encode_dev(dev_t dev)
{
	return (major(dev) << 8) | minor(dev);
}

static inline __u32 new_encode_dev(dev_t dev)
{
	unsigned major = major(dev);
	unsigned minor = minor(dev);
	return (minor & 0xff) | (major << 8) | ((minor & ~0xff) << 12);
}

int do_create (ext2_filsys e2fs, const char *path, mode_t mode, dev_t dev, const char *fastsymlink)
{
	int rt;
	time_t tm;
	errcode_t rc;

	char *p_path;
	char *r_path;

	ext2_ino_t ino;
	struct ext2_inode inode;
	ext2_ino_t n_ino;

	struct fuse_context *ctx;

	debugf("enter");
	debugf("path = %s, mode: 0%o", path, mode);

	rt=do_check_split(path, &p_path, &r_path);

	debugf("parent: %s, child: %s", p_path, r_path);

	rt = do_readinode(e2fs, p_path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", p_path);
		free_split(p_path, r_path);
		return rt;
	}

	rc = ext2fs_new_inode(e2fs, ino, mode, 0, &n_ino);
	if (rc) {
		debugf("ext2fs_new_inode(ep.fs, ino, mode, 0, &n_ino); failed");
		free_split(p_path, r_path);
		return -ENOMEM;
	}

	do {
		debugf("calling ext2fs_link(e2fs, %d, %s, %d, %d);", ino, r_path, n_ino, do_modetoext2lag(mode));
		rc = ext2fs_link(e2fs, ino, r_path, n_ino, do_modetoext2lag(mode));
		if (rc == EXT2_ET_DIR_NO_SPACE) {
			debugf("calling ext2fs_expand_dir(e2fs, &d)", ino);
			if (ext2fs_expand_dir(e2fs, ino)) {
				debugf("error while expanding directory %s (%d)", p_path, ino);
				free_split(p_path, r_path);
				return -ENOSPC;
			}
		}
	} while (rc == EXT2_ET_DIR_NO_SPACE);
	if (rc) {
		debugf("ext2fs_link(e2fs, %d, %s, %d, %d); failed", ino, r_path, n_ino, do_modetoext2lag(mode));
		free_split(p_path, r_path);
		return -EIO;
	}

	if (ext2fs_test_inode_bitmap(e2fs->inode_map, n_ino)) {
		debugf("inode already set");
	}

	ext2fs_inode_alloc_stats2(e2fs, n_ino, +1, 0);
	memset(&inode, 0, sizeof(inode));
	tm = e2fs->now ? e2fs->now : time(NULL);
	inode.i_mode = mode;
	inode.i_atime = inode.i_ctime = inode.i_mtime = tm;
	inode.i_links_count = 1;
	inode.i_size = 0;
	ctx = fuse_get_context();
	if (ctx) {
		inode.i_uid = ctx->uid;
		inode.i_gid = ctx->gid;
	}

	if (S_ISCHR(mode) || S_ISBLK(mode)) {
		if (old_valid_dev(dev))
			inode.i_block[0]= ext2fs_cpu_to_le32(old_encode_dev(dev));
		else
			inode.i_block[1]= ext2fs_cpu_to_le32(new_encode_dev(dev));
	}

	if (S_ISLNK(mode) && fastsymlink != NULL) {
		inode.i_size = strlen(fastsymlink);
		strncpy((char *)&(inode.i_block[0]),fastsymlink,
				(EXT2_N_BLOCKS * sizeof(inode.i_block[0])));
	}

	rc = ext2fs_write_new_inode(e2fs, n_ino, &inode);
	if (rc) {
		debugf("ext2fs_write_new_inode(e2fs, n_ino, &inode);");
		free_split(p_path, r_path);
		return -EIO;
	}

	/* update parent dir */
	rt = do_readinode(e2fs, p_path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); dailed", p_path);
		free_split(p_path, r_path);
		return -EIO;
	}
	inode.i_ctime = inode.i_mtime = tm;
	rc = ext2fs_write_inode(e2fs, ino, &inode);
	if (rc) {
		debugf("ext2fs_write_inode(e2fs, ino, &inode); failed");
		free_split(p_path, r_path);
		return -EIO;
	}

	free_split(p_path, r_path);

	debugf("leave");
	return 0;
}

int op_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int rt;
	ext2_filsys e2fs = current_ext2fs();

	debugf("enter");
	debugf("path = %s, mode: 0%o", path, mode);

	if (op_open(path, fi) == 0) {
		debugf("leave");
		return 0;
	}

	rt = do_create(e2fs, path, mode, 0, NULL);
	if (rt != 0) {
		return rt;
	}

	if (op_open(path, fi)) {
		debugf("op_open(path, fi); failed");
		return -EIO;
	}

	debugf("leave");
	return 0;
}
