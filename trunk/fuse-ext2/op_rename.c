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

static int fix_dotdot_proc (ext2_ino_t dir EXT2FS_ATTR((unused)),
		int entry EXT2FS_ATTR((unused)),
		struct ext2_dir_entry *dirent,
		int offset EXT2FS_ATTR((unused)),
		int blocksize EXT2FS_ATTR((unused)),
		char *buf EXT2FS_ATTR((unused)), void *private)
{
	ext2_ino_t *p_dotdot = (ext2_ino_t *) private;

	debugf("enter");
	debugf("walking on: %s", dirent->name);

	if ((dirent->name_len & 0xFF) == 2 && strncmp(dirent->name, "..", 2) == 0) {
		dirent->inode = *p_dotdot;

		debugf("leave (found '..')");
		return DIRENT_ABORT | DIRENT_CHANGED;
	} else {
		debugf("leave");
		return 0;
	}
}

static int do_fix_dotdot(ext2_filsys e2fs, ext2_ino_t ino, ext2_ino_t dotdot)
{
	errcode_t rc;

	debugf("enter");
	rc = ext2fs_dir_iterate2(e2fs, ino, DIRENT_FLAG_INCLUDE_EMPTY, 
			0, fix_dotdot_proc, &dotdot);
	if (rc) {
		debugf("while iterating over directory");
		return -EIO;
	}
	debugf("leave");
	return 0;
}

int op_rename(const char *source, const char *dest)
{
	int rt;
	errcode_t rc;
	int destrt;

	char *p_src;
	char *r_src;
	char *p_dest;
	char *r_dest;

	ext2_ino_t src_ino;
	ext2_ino_t dest_ino;
	ext2_ino_t d_src_ino;
	ext2_ino_t d_dest_ino;
	struct ext2_vnode *src_vnode;
	struct ext2_inode *src_inode;
	struct ext2_vnode *dest_vnode;
	struct ext2_inode *dest_inode;
	struct ext2_inode d_src_inode;
	struct ext2_inode d_dest_inode;
	ext2_filsys e2fs = current_ext2fs();

	debugf("source: %s, dest: %s", source, dest);

	rt = do_check_split(source, &p_src, &r_src);
	if (rt != 0) {
		debugf("do_check(%s); failed", source);
		return rt;
	}

	debugf("src_parent: %s, src_child: %s", p_src, r_src);

	rt = do_check_split(dest, &p_dest, &r_dest);
	if (rt != 0) {
		debugf("do_check(%s); failed", dest);
		goto out_free_src;
	}

	debugf("dest_parent: %s, dest_child: %s", p_dest, r_dest);

	rt = do_readinode(e2fs, p_src, &d_src_ino, &d_src_inode);
	if (rt != 0) {
		debugf("do_readinode(%s, &d_src_ino, &d_src_inode); failed", p_src);
		goto out_free;
	}

	rt = do_readinode(e2fs, p_dest, &d_dest_ino, &d_dest_inode);
	if (rt != 0) {
		debugf("do_readinode(%s, &d_dest_ino, &d_dest_inode); failed", p_dest);
		goto out_free;
	}

	rt = do_readvnode(e2fs, source, &src_ino, &src_vnode);
	if (rt != 0) {
		debugf("do_readvnode(%s, &src_ino, &src_vnode); failed", source);
		goto out_free;
	}

	/* dest == ENOENT is okay */
	destrt = do_readvnode(e2fs, dest, &dest_ino, &dest_vnode);
	if (rt != 0 && rt != -ENOENT) {
		debugf("do_readinode(%s, &dest_ino, &dest_inode); failed", dest);
		goto out_vsrc;
	}

	src_inode = vnode2inode(src_vnode);
	dest_inode = vnode2inode(dest_vnode);

	/* If  oldpath  and  newpath are existing hard links referring to the same
		 file, then rename() does nothing, and returns a success status. */
	if (destrt == 0 && src_ino == dest_ino) {
		rt = 0;
		goto out;
	}

	/* error cases */
	/* EINVAL The  new  pathname  contained a path prefix of the old:
		 this should be checked by fuse */
	if (destrt == 0) {
		if (LINUX_S_ISDIR(dest_inode->i_mode)) {
			/* EISDIR newpath  is  an  existing directory, but oldpath is not a direc‐
				 tory. */
		 if (!(LINUX_S_ISDIR(src_inode->i_mode))) {
			 debugf("newpath is dir && oldpath is not a dir -> EISDIR");
			 rt = -EISDIR;
			 goto out;
		 }
		 /* ENOTEMPTY newpath is a non-empty  directory */
		 rt = do_check_empty_dir(e2fs, dest_ino);
		 if (rt != 0) {
			 debugf("do_check_empty_dir dest %s failed",dest);
			 goto out;
		 }
		}
		/* ENOTDIR: oldpath  is a directory, and newpath exists but is not a 
			 directory */
		if (LINUX_S_ISDIR(src_inode->i_mode) &&
				!(LINUX_S_ISDIR(dest_inode->i_mode))) {
			debugf("oldpath is dir && newpath is not a dir -> ENOTDIR");
			rt = -ENOTDIR;
			goto out;
		}
	}

	/* Step 1: if destination exists: delete it */
	if (destrt == 0) {
		/* unlink in both cases */
		rc = ext2fs_unlink(e2fs, d_dest_ino, r_dest, dest_ino, 0);
		if (rc) {
			debugf("ext2fs_unlink(e2fs, %d, %s, %d, 0); failed", d_dest_ino, r_dest, dest_ino);
			rt = -EIO;
			goto out;
		}

		if (LINUX_S_ISDIR(dest_inode->i_mode)) {
			/* empty dir */
			rt = do_killfilebyinode(e2fs, dest_ino, dest_inode);
			if (rt) {
				debugf("do_killfilebyinode(r_ino, &r_inode); failed");
				goto out;
			}
			rt = do_readinode(e2fs, p_dest, &d_dest_ino, &d_dest_inode);
			if (rt) {
				debugf("do_readinode(p_dest, &d_dest_ino, &d_dest_inode); failed");
				goto out_free;
			}
			if (d_dest_inode.i_links_count > 1) {
				d_dest_inode.i_links_count--;
			}
			rc = ext2fs_write_inode(e2fs, d_dest_ino, &d_dest_inode);
			if (rc) {
				debugf("ext2fs_write_inode(e2fs, ino, inode); failed");
				rt = -EIO;
			}
			vnode_put(dest_vnode,0);
		} else {
			/* file */
			if (dest_inode->i_links_count > 0) {
				dest_inode->i_links_count -= 1;
			}
			rc = vnode_put(dest_vnode,1);
			if (rc) {
				debugf("vnode_put(dest_vnode,1); failed");
				rt = -EIO;
				goto out_vsrc;
			}
		}
	}
  	
	/* Step 2: add the link */
	do {
		debugf("calling ext2fs_link(e2fs, %d, %s, %d, %d);", d_dest_ino, r_dest, src_ino, do_modetoext2lag(src_inode->i_mode));
		rc = ext2fs_link(e2fs, d_dest_ino, r_dest, src_ino, do_modetoext2lag(src_inode->i_mode));
		if (rc == EXT2_ET_DIR_NO_SPACE) {
			debugf("calling ext2fs_expand_dir(e2fs, &d)", src_ino);
			if (ext2fs_expand_dir(e2fs, d_dest_ino)) {
				debugf("error while expanding directory %s (%d)", p_dest, d_dest_ino);
				rt = -ENOSPC;
				goto out_vsrc;
			}
			/* ext2fs_expand_dir changes d_dest_inode */
			rt = do_readinode(e2fs, p_dest, &d_dest_ino, &d_dest_inode);
			if (rt != 0) {
				debugf("do_readinode(%s, &d_dest_ino, &d_dest_inode); failed", p_dest);
				goto out_vsrc;
			}
		}
	} while (rc == EXT2_ET_DIR_NO_SPACE);
	if (rc != 0) {
		debugf("ext2fs_link(e2fs, %d, %s, %d, %d); failed", d_dest_ino, r_dest, src_ino, do_modetoext2lag(src_inode->i_mode));
		rt = -EIO;
		goto out_vsrc;
	}

	/* Special case: if moving dir across different parents 
		 fix counters and '..' */
	if (LINUX_S_ISDIR(src_inode->i_mode) && d_src_ino != d_dest_ino) {
		d_dest_inode.i_links_count++;
		if (d_src_inode.i_links_count > 1)
			d_src_inode.i_links_count--;
		rc = ext2fs_write_inode(e2fs, d_src_ino, &d_src_inode);
		if (rc != 0) {
			debugf("ext2fs_write_inode(e2fs, src_ino, &src_inode); failed");
			rt = -EIO;
			goto out_vsrc;
		}
		rt = do_fix_dotdot(e2fs, src_ino, d_dest_ino);
		if (rt != 0) {
			debugf("do_fix_dotdot failed");
			goto out_vsrc;
		}
	}

	/* utimes and inodes update */
	d_dest_inode.i_mtime = d_dest_inode.i_ctime = src_inode->i_ctime = e2fs->now ? e2fs->now : time(NULL);
	rc = ext2fs_write_inode(e2fs, d_dest_ino, &d_dest_inode);
	if (rc != 0) {
		debugf("ext2fs_write_inode(e2fs, d_dest_ino, &d_dest_inode); failed");
		rt = -EIO;
		goto out_vsrc;
	}
	rc = vnode_put(src_vnode, 1);
	if (rc != 0) {
		debugf("vnode_put(src_vnode,1); failed");
		rt = -EIO;
		goto out_free;
	}
	debugf("done");

	/* Step 3: delete the source */

	rc = ext2fs_unlink(e2fs, d_src_ino, r_src, src_ino, 0);
	if (rc) {
		debugf("while unlinking src ino %d", (int) src_ino);
		rt = -EIO;
		goto out_free;
	}

	free_split(p_src, r_src);
	free_split(p_dest, r_dest);
	return 0;
out:
	if (destrt == 0)
		vnode_put(dest_vnode,0);
out_vsrc:
	vnode_put(src_vnode,0);
out_free:
	free_split(p_src, r_src);
	free_split(p_dest, r_dest);
out_free_src:
	free_split(p_src, r_src);

	return rt;
}
