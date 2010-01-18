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

#define VOLNAME_SIZE_MAX 16

int do_probe (struct extfs_data *opts)
{
	errcode_t rc;
	ext2_filsys e2fs;

	debugf_main("enter");

	rc = ext2fs_open(opts->device, EXT2_FLAG_RW, 0, 0, unix_io_manager, &e2fs);
	if (rc) {
		debugf_main("Error while trying to open %s (rc=%d)", opts->device, rc);
		return -1;
	}
#if 0
	rc = ext2fs_read_bitmaps(e2fs);
	if (rc) {
		debugf_main("Error while reading bitmaps (rc=%d)", rc);
		ext2fs_close(e2fs);
		return -2;
	}
#endif
	if (e2fs->super != NULL) {
		opts->volname = (char *) malloc(sizeof(char) * (VOLNAME_SIZE_MAX + 1));
		if (opts->volname != NULL) {
			memset(opts->volname, 0, sizeof(char) * (VOLNAME_SIZE_MAX + 1));
			strncpy(opts->volname, e2fs->super->s_volume_name, VOLNAME_SIZE_MAX);
			opts->volname[VOLNAME_SIZE_MAX] = '\0';
		}
	}
	ext2fs_close(e2fs);

	debugf_main("leave");
	return 0;
}
