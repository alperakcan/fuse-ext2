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

int do_check (const char *path)
{
	char *basename_path;
	basename_path = strrchr(path, '/');
	if (basename_path == NULL) {
		debugf("this should not happen %s", path);
		return -ENOENT;
	}
	basename_path++;
	if (strlen(basename_path) > 255) {
		debugf("basename exceeds 255 characters %s",path);
		return -ENAMETOOLONG;
	}
	return 0;
}

int do_check_split (const char *path, char **dirname, char **basename)
{
	char *tmp;
	char *cpath = strdup(path);
	tmp = strrchr(cpath, '/');
	if (tmp == NULL) {
		debugf("this should not happen %s", path);
		free(cpath);
		return -ENOENT;
	}
	*tmp='\0';
	tmp++;
	if (strlen(tmp) > 255) {
		debugf("basename exceeds 255 characters %s",path);
		free(cpath);
		return -ENAMETOOLONG;
	}
	*dirname=cpath;
	*basename=tmp;
	return 0;
}

void free_split (char *dirname, char *basename)
{
	free(dirname);
}

