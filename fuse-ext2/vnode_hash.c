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

//#define VNODE_DEBUG 1

#define VNODE_HASH_SIZE 256
#define VNODE_HASH_MASK (VNODE_HASH_SIZE-1)

#if !defined(VNODE_DEUG)
#undef debugf
#define debugf(a...) do { } while (0)
#endif

struct ext2_vnode {
	struct ext2_inode inode;
	ext2_filsys e2fs;
	ext2_ino_t ino;
	int count;
	struct ext2_vnode **pprevhash,*nexthash;
};

static struct ext2_vnode *ht_head[VNODE_HASH_SIZE];

static inline struct ext2_vnode * vnode_alloc (void)
{
	return (struct ext2_vnode *) malloc(sizeof(struct ext2_vnode));
}

static inline void vnode_free (struct ext2_vnode *vnode)
{
	free(vnode);
}

static inline int vnode_hash_key (ext2_filsys e2fs, ext2_ino_t ino)
{
	return ((int) e2fs + ino) & VNODE_HASH_MASK;
}

struct ext2_vnode * vnode_get (ext2_filsys e2fs, ext2_ino_t ino)
{
	int hash_key = vnode_hash_key(e2fs,ino);
	struct ext2_vnode *rv = ht_head[hash_key];

	while (rv != NULL && rv->ino != ino) {
		rv = rv->nexthash;
	}
	if (rv != NULL) {
		rv->count++;
		debugf("increased hash:%p use count:%d", rv, rv->count);
		return rv;
	} else {
		struct ext2_vnode *new = vnode_alloc();
		if (new != NULL) {
			errcode_t rc;
			rc = ext2fs_read_inode(e2fs, ino, &new->inode);
			if (rc != 0) {
				vnode_free(new);
				debugf("leave error");
				return NULL;
			}
			new->e2fs = e2fs;
			new->ino = ino;
			new->count = 1;
			if (ht_head[hash_key] != NULL) {
				ht_head[hash_key]->pprevhash = &(new->nexthash);
			}
			new->nexthash = ht_head[hash_key];
			new->pprevhash = &(ht_head[hash_key]);
			ht_head[hash_key] = new;
			debugf("added hash:%p", new);
		}
		return new;
	}
}

int vnode_put (struct ext2_vnode *vnode, int dirty)
{
	int rt = 0;
	vnode->count--;
	if (vnode->count <= 0) {
		debugf("deleting hash:%p", vnode);
		if (vnode->inode.i_links_count < 1) {
			rt = do_killfilebyinode(vnode->e2fs, vnode->ino, &vnode->inode);
		} else if (dirty) {
			rt = ext2fs_write_inode(vnode->e2fs, vnode->ino, &(vnode->inode));
		}
		*(vnode->pprevhash) = vnode->nexthash;
		if (vnode->nexthash) {
			vnode->nexthash->pprevhash = vnode->pprevhash;
		}
		vnode_free(vnode);
	} else if (dirty) {
		rt = ext2fs_write_inode(vnode->e2fs, vnode->ino, &(vnode->inode));
	}
	return rt;
}
