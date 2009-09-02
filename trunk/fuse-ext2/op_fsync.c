/**
 * Copyright (c) 2008-2009 Alper Akcan <alper.akcan@gmail.com>
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

int op_fsync (const char *path, int datasync, struct fuse_file_info *fi)
{
	errcode_t rc;
	ext2_filsys e2fs = current_ext2fs();

	debugf("enter");
	debugf("path = %s (%p)", path, fi);
	
	rc = ext2fs_flush(e2fs);
	if (rc) {
		return -EIO;
	}

	debugf("leave");
	return 0;
}
