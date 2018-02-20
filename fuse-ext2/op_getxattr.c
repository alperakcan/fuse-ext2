/**
 * @file
 * @brief EXT2 getxattr support
 *
 * @date 14.01.2015
 * @author Alexander Kalmuk
 */

#include "fuse-ext2.h"

#if !defined(ENOATTR)
#define ENOATTR ENODATA
#endif

static int do_getxattr(ext2_filsys e2fs, struct ext2_inode *node, const char *name,
		char *value, size_t size);

int op_getxattr(const char *path, const char *name, char *value, size_t size) {
	int rt;
	ext2_ino_t ino;
	struct ext2_inode inode;
	ext2_filsys e2fs = current_ext2fs();

	debugf("enter");
	debugf("path = %s", path);
	debugf("path = %s, %s, %p, %d", path, name, value, size);

	rt = do_check(path);
	if (rt != 0) {
		debugf("do_check(%s); failed", path);
		return rt;
	}

	rt = do_readinode(e2fs, path, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", path);
		return rt;
	}

	rt = do_getxattr(e2fs, &inode, name, value, size);
	if (rt < 0) {
		debugf("do_getxattr(e2fs, inode, %s, value, %d); failed", name, size);
		return rt;
	}

	debugf("leave");
	return rt;
}

/**
 * Name is of format namespace:attribute. This function return namespace as @p name_index
 * and attribute as @p attr_name
 *
 * TODO support trusted, system and security attributes
 */
static int parse_name(const char *name, int *name_index, char **attr_name) {
	char namespace[16];
	char *attr_name_str;

	memcpy(namespace, name, sizeof namespace);

	attr_name_str = strchr(namespace, '.');
	if (!attr_name) {
		return -ENOTSUP;
	} else {
		*attr_name_str = 0;
		*attr_name = ++attr_name_str;
	}

	if (!strcmp(namespace, "user")) {
		*name_index = 1;
		return 0;
	}

	return -ENOTSUP;
}

static int do_getxattr(ext2_filsys e2fs, struct ext2_inode *node, const char *name,
		char *value, size_t size) {
	char *buf, *attr_start;
	struct ext2_ext_attr_entry *entry;
	char *entry_name, *value_name;
	int name_index;
	int res;

	res = parse_name(name, &name_index, &value_name);
	if (res < 0) {
		return res;
	}

	buf = malloc(e2fs->blocksize);
	if (!buf) {
		return -ENOMEM;
	}
	ext2fs_read_ext_attr(e2fs, node->i_file_acl, buf);

	attr_start = buf + sizeof(struct ext2_ext_attr_header);
	entry = (struct ext2_ext_attr_entry *) attr_start;
	res = -ENOATTR;

	while (!EXT2_EXT_IS_LAST_ENTRY(entry)) {
		entry_name = (char *)entry + sizeof(struct ext2_ext_attr_entry);

		if (name_index == entry->e_name_index &&
				entry->e_name_len == strlen(value_name)) {
			if (!strncmp(entry_name, value_name, entry->e_name_len)) {
				if (size > 0) {
					memcpy(value, buf + entry->e_value_offs, entry->e_value_size);
				}
				res = entry->e_value_size;
				break;
			}
		}
		entry = EXT2_EXT_ATTR_NEXT(entry);
	}

	free(buf);
	return res;
}
