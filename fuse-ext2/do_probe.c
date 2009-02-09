/**
 * Copyright (c) 2008-2009 Alper Akcan <alper.akcan@gmail.com>
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

int do_probe (void)
{
	errcode_t rc;

	debugf("enter");

	priv.name = opts.device;
	rc = ext2fs_open(priv.name, EXT2_FLAG_RW, 0, 0, unix_io_manager, &priv.fs);
	if (rc) {
		debugf("Error while trying to open %s", priv.name);
		return -1;
	}
	rc = ext2fs_read_bitmaps(priv.fs);
	if (rc) {
		debugf("Error while reading bitmaps");
		ext2fs_close(priv.fs);
		return -2;
	}
	ext2fs_close(priv.fs);

	debugf("leave");
	return 0;
}
