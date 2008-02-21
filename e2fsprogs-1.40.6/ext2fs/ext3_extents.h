/*
 * Copyright (c) 2003,2004 Cluster File Systems, Inc, info@clusterfs.com
 * Written by Alex Tomas <alex@clusterfs.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public Licens
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-
 */

#ifndef _LINUX_EXT3_EXTENTS
#define _LINUX_EXT3_EXTENTS

/*
 * with AGRESSIVE_TEST defined capacity of index/leaf blocks
 * become very little, so index split, in-depth growing and
 * other hard changes happens much more often
 * this is for debug purposes only
 */
#define AGRESSIVE_TEST_

/*
 * if CHECK_BINSEARCH defined, then results of binary search
 * will be checked by linear search
 */
#define CHECK_BINSEARCH_

/*
 * if EXT_DEBUG is defined you can use 'extdebug' mount option
 * to get lots of info what's going on
 */
//#define EXT_DEBUG
#ifdef EXT_DEBUG
#define ext_debug(tree,fmt,a...) 			\
do {							\
	if (test_opt((tree)->inode->i_sb, EXTDEBUG))	\
		printk(fmt, ##a);			\
} while (0);
#else
#define ext_debug(tree,fmt,a...)
#endif

/*
 * if EXT_STATS is defined then stats numbers are collected
 * these number will be displayed at umount time
 */
#define EXT_STATS_


#define EXT3_ALLOC_NEEDED	3	/* block bitmap + group desc. + sb */

/*
 * ext3_inode has i_block array (total 60 bytes)
 * first 4 bytes are used to store:
 *  - tree depth (0 mean there is no tree yet. all extents in the inode)
 *  - number of alive extents in the inode
 */

/*
 * this is extent on-disk structure
 * it's used at the bottom of the tree
 */
struct ext3_extent {
	__u32	ee_block;	/* first logical block extent covers */
	__u16	ee_len;		/* number of blocks covered by extent */
	__u16	ee_start_hi;	/* high 16 bits of physical block */
	__u32	ee_start;	/* low 32 bigs of physical block */
};

/*
 * this is index on-disk structure
 * it's used at all the levels, but the bottom
 */
struct ext3_extent_idx {
	__u32	ei_block;	/* index covers logical blocks from 'block' */
	__u32	ei_leaf;	/* pointer to the physical block of the next *
				 * level. leaf or next index could bet here */
	__u16	ei_leaf_hi;	/* high 16 bits of physical block */
	__u16	ei_unused;
};

/*
 * each block (leaves and indexes), even inode-stored has header
 */
struct ext3_extent_header {
	__u16	eh_magic;	/* probably will support different formats */
	__u16	eh_entries;	/* number of valid entries */
	__u16	eh_max;		/* capacity of store in entries */
	__u16	eh_depth;	/* has tree real underlaying blocks? */
	__u32	eh_generation;	/* generation of the tree */
};

#define EXT3_EXT_MAGIC		0xf30a

/*
 * array of ext3_ext_path contains path to some extent
 * creation/lookup routines use it for traversal/splitting/etc
 * truncate uses it to simulate recursive walking
 */
struct ext3_ext_path {
	__u32				p_block;
	__u16				p_depth;
	struct ext3_extent		*p_ext;
	struct ext3_extent_idx		*p_idx;
	struct ext3_extent_header	*p_hdr;
	struct buffer_head		*p_bh;
};

/*
 * structure for external API
 */

#define EXT_CONTINUE	0
#define EXT_BREAK	1
#define EXT_REPEAT	2


#define EXT_MAX_BLOCK	0xffffffff
#define EXT_CACHE_MARK	0xffff


#define EXT_FIRST_EXTENT(__hdr__) \
	((struct ext3_extent *) (((char *) (__hdr__)) +		\
				 sizeof(struct ext3_extent_header)))
#define EXT_FIRST_INDEX(__hdr__) \
	((struct ext3_extent_idx *) (((char *) (__hdr__)) +	\
				     sizeof(struct ext3_extent_header)))
#define EXT_HAS_FREE_INDEX(__path__) \
	((__path__)->p_hdr->eh_entries < (__path__)->p_hdr->eh_max)
#define EXT_LAST_EXTENT(__hdr__) \
	(EXT_FIRST_EXTENT((__hdr__)) + (__hdr__)->eh_entries - 1)
#define EXT_LAST_INDEX(__hdr__) \
	(EXT_FIRST_INDEX((__hdr__)) + (__hdr__)->eh_entries - 1)
#define EXT_MAX_EXTENT(__hdr__) \
	(EXT_FIRST_EXTENT((__hdr__)) + (__hdr__)->eh_max - 1)
#define EXT_MAX_INDEX(__hdr__) \
	(EXT_FIRST_INDEX((__hdr__)) + (__hdr__)->eh_max - 1)

#define EXT_ROOT_HDR(tree) \
	((struct ext3_extent_header *) (tree)->root)
#define EXT_BLOCK_HDR(bh) \
	((struct ext3_extent_header *) (bh)->b_data)
#define EXT_DEPTH(_t_)	\
	(((struct ext3_extent_header *)((_t_)->root))->eh_depth)
#define EXT_GENERATION(_t_)	\
	(((struct ext3_extent_header *)((_t_)->root))->eh_generation)


#define EXT_ASSERT(__x__) if (!(__x__)) BUG();


/*
 * this structure is used to gather extents from the tree via ioctl
 */
struct ext3_extent_buf {
	unsigned long start;
	int buflen;
	void *buffer;
	void *cur;
	int err;
};

/*
 * this structure is used to collect stats info about the tree
 */
struct ext3_extent_tree_stats {
	int depth;
	int extents_num;
	int leaf_num;
};

#ifdef __KERNEL__
/*
 * ext3_extents_tree is used to pass initial information
 * to top-level extents API
 */
struct ext3_extents_helpers;
struct ext3_extents_tree {
	struct inode *inode;	/* inode which tree belongs to */
	void *root;		/* ptr to data top of tree resides at */
	void *buffer;		/* will be passed as arg to ^^ routines	*/
	int buffer_len;
	void *private;
	struct ext3_extent *cex;/* last found extent */
	struct ext3_extents_helpers *ops;
};

struct ext3_extents_helpers {
	int (*get_write_access)(handle_t *h, void *buffer);
	int (*mark_buffer_dirty)(handle_t *h, void *buffer);
	int (*mergable)(struct ext3_extent *ex1, struct ext3_extent *ex2);
	int (*remove_extent_credits)(struct ext3_extents_tree *,
					struct ext3_extent *, unsigned long,
					unsigned long);
	int (*remove_extent)(struct ext3_extents_tree *,
				struct ext3_extent *, unsigned long,
				unsigned long);
	int (*new_block)(handle_t *, struct ext3_extents_tree *,
				struct ext3_ext_path *, struct ext3_extent *,
				int *);
};

/*
 * to be called by ext3_ext_walk_space()
 * negative retcode - error
 * positive retcode - signal for ext3_ext_walk_space(), see below
 * callback must return valid extent (passed or newly created)
 */
typedef int (*ext_prepare_callback)(struct ext3_extents_tree *,
					struct ext3_ext_path *,
					struct ext3_extent *, int);
void ext3_init_tree_desc(struct ext3_extents_tree *, struct inode *);
extern int ext3_extent_tree_init(handle_t *, struct ext3_extents_tree *);
extern int ext3_ext_calc_credits_for_insert(struct ext3_extents_tree *, struct ext3_ext_path *);
extern int ext3_ext_insert_extent(handle_t *, struct ext3_extents_tree *, struct ext3_ext_path *, struct ext3_extent *);
extern int ext3_ext_walk_space(struct ext3_extents_tree *, unsigned long, unsigned long, ext_prepare_callback);
extern int ext3_ext_remove_space(struct ext3_extents_tree *, unsigned long, unsigned long);
extern struct ext3_ext_path * ext3_ext_find_extent(struct ext3_extents_tree *, int, struct ext3_ext_path *);

static inline void
ext3_ext_invalidate_cache(struct ext3_extents_tree *tree)
{
	if (tree->cex)
		tree->cex->ee_len = 0;
}
#endif /* __KERNEL__ */


#endif /* _LINUX_EXT3_EXTENTS */

