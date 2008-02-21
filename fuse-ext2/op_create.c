
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

int op_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int rt;
	char *tmp;
	char *path_parent;
	char *path_real;

	errcode_t rc;
	ext2_ino_t ino;
	struct ext2_inode inode;
	ext2_ino_t n_ino;

	struct fuse_context *ctx;
	
	debugf("enter");
	debugf("path = %s, mode: 0%o", path, mode);
	
	if (op_open(path, fi) == 0) {
		debugf("leave");
		return 0;
	}

	path_parent = strdup(path);
	if (path_parent == NULL) {
		debugf("strdup(%s); failed", path);
		return -ENOMEM;
	}
	tmp = strrchr(path_parent, '/');
	if (tmp == NULL) {
		debugf("this should not happen");
		free(path_parent);
		return -ENOENT;
	}
	*tmp = '\0';
	path_real = tmp + 1;
	debugf("parent: %s, child: %s", path_parent, path_real);
	
	rt = do_readinode(path_parent, &ino, &inode);
	if (rt) {
		debugf("do_readinode(%s, &ino, &inode); failed", path_parent);
		free(path_parent);
		return rt;
	}
	
	rc = ext2fs_new_inode(priv.fs, ino, mode, 0, &n_ino);
	if (rc) {
		debugf("ext2fs_new_inode(ep.fs, ino, mode, 0, &n_ino); failed");
		return -ENOMEM;
	}

	do {
		debugf("calling ext2fs_link(priv.fs, %d, %s, %d, %d);", ino, path_real, n_ino, do_modetoext2lag(mode));
		rc = ext2fs_link(priv.fs, ino, path_real, n_ino, do_modetoext2lag(mode));
		if (rc == EXT2_ET_DIR_NO_SPACE) {
			debugf("calling ext2fs_expand_dir(priv.fs, &d)", ino);
			if (ext2fs_expand_dir(priv.fs, ino)) {
				debugf("error while expanding directory %s (%d)", path_parent, ino);
				free(path_parent);
				return -ENOSPC;
			}
		}
	} while (rc == EXT2_ET_DIR_NO_SPACE);
	if (rc) {
		debugf("ext2fs_mkdir(priv.fs, %d, 0, %s); failed (%d)", ino, path_real, rc);
		debugf("priv.fs: %p, priv.fs->inode_map: %p", priv.fs, priv.fs->inode_map);
		free(path_parent);
		return -EIO;
	}
	free(path_parent);
	
	if (ext2fs_test_inode_bitmap(priv.fs->inode_map, n_ino)) {
		debugf("inode already set");
	}

	ext2fs_inode_alloc_stats2(priv.fs, n_ino, +1, 0);
	memset(&inode, 0, sizeof(inode));
	inode.i_mode = mode;
	inode.i_atime = inode.i_ctime = inode.i_mtime = time(NULL);
	inode.i_links_count = 1;
	inode.i_size = 0;
	ctx = fuse_get_context();
	if (ctx) {
		inode.i_uid = ctx->uid;
		inode.i_gid = ctx->gid;
	}
	
	rc = ext2fs_write_new_inode(priv.fs, n_ino, &inode);
	if (rc) {
		debugf("ext2fs_write_new_inode(e2fs, n_ino, &inode);");
		return -EIO;
	}
	
	if (op_open(path, fi)) {
		debugf("op_open(path, fi); failed");
		return -EIO;
	}

	debugf("leave");
	return 0;
}