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

void * op_init (struct fuse_conn_info *conn)
{
	errcode_t rc;
	struct fuse_context *cntx=fuse_get_context();
	struct extfs_data *e2data=cntx->private_data;

	debugf("enter %s", e2data->device);

	rc = ext2fs_open(e2data->device, 
			(e2data->readonly) ? 0 : EXT2_FLAG_RW,
			0, 0, unix_io_manager, &e2data->e2fs);
	if (rc) {
		debugf("Error while trying to open %s", e2data->device);
		exit(1);
	}
#if 1
	if (e2data->readonly != 1)
#endif
	rc = ext2fs_read_bitmaps(e2data->e2fs);
	if (rc) {
		debugf("Error while reading bitmaps");
		ext2fs_close(e2data->e2fs);
		exit(1);
	}
	debugf("FileSystem %s", (e2data->e2fs->flags & EXT2_FLAG_RW) ? "Read&Write" : "ReadOnly");

	debugf("leave");

	return e2data;
}
