/**
 * Copyright (c) 2008-2010 Alper Akcan <alper.akcan@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include <errno.h>

#define debugf(a...) { \
	printf("fuse-ext2.uninstall: "); \
	printf(a); \
	printf(" (%s) [%s (%s:%d)]\n", strerror(errno), __FUNCTION__, __FILE__, __LINE__); \
}

static int verbose = 0;
static int uninstall = 0;

static char *files[] = {
	"/Library/Receipts/fuse-ext2.pkg",
	"/Library/Filesystems/fuse-ext2.fs",
	"/System/Library/Filesystems/fuse-ext2.fs",
	"/Library/PreferencePanes/fuse-ext2.prefPane",
	"/System/Library/PreferencePanes/fuse-ext2.prefPane",
	"/usr/local/bin/fuse-ext2",
	"/usr/local/bin/fuse-ext2.wait",
	"/usr/local/bin/fuse-ext2.probe",
	"/usr/local/bin/fuse-ext2.mke2fs",
	"/usr/local/bin/fuse-ext2.e2label",
	"/usr/local/bin/fuse-ext2.install",
	"/usr/local/bin/fuse-ext2.uninstall",
	"/usr/local/lib/pkgconfig/fuse-ext2.pc",
	"/usr/local/share/man/man1/fuse-ext2.1",
	NULL,
};

static int rm_file (const char *path)
{
	printf("removing file '%s'\n", path);
	if (verbose == 0 && unlink(path) != 0) {
		debugf("unlink failed for '%s'", path);
	}
	return 0;
}

static int rm_directory (const char *path)
{
	DIR *dp;
	char *p;
	struct stat stbuf;
	struct dirent *current;
	dp = opendir(path);
	if (dp == NULL) {
		debugf("opendir() failed for '%s'", path);
		if (errno == ENOENT) {
			return 0;
		}
		return -1;
	}
	while ((current = readdir(dp)) != NULL) {
		if (strcmp(current->d_name, ".") == 0 ||
		    strcmp(current->d_name, "..") == 0) {
			continue;
		}
		p = (char *) malloc(sizeof(char) * (strlen(path) + 1 + strlen(current->d_name) + 1));
		if (p == NULL) {
			debugf("malloc failed for '%s/%s'", path, current->d_name);
			continue;
		}
		sprintf(p, "%s/%s", path, current->d_name);
		if (lstat(p, &stbuf) != 0) {
			debugf("lstat failed for '%s'", p);
			free(p);
			continue;
		}
		if (S_ISDIR(stbuf.st_mode)) {
			if (rm_directory(p) != 0) {
				debugf("rm_directory() failed for '%s'", p);
			}
		} else {
			if (rm_file(p) != 0) {
				debugf("rm_file() failed for '%s'", p);
			}
		}
		free(p);
	}
	closedir(dp);
	printf("removing directory '%s'\n", path);
	if (verbose == 0 && rmdir(path) != 0) {
		debugf("rmdir() failed for '%s'", path);
	}
	return 0;
}

static int rm_path (const char *path)
{
	struct stat stbuf;
	if (lstat(path, &stbuf) != 0) {
		debugf("lstat failed for '%s'", path);
		return -1;
	}
	if (S_ISDIR(stbuf.st_mode)) {
		return rm_directory(path);
	} else {
		return rm_file(path);
	}
	return -1;
}

static void print_help (const char *pname)
{
	printf("%s usage;\n", pname);
	printf("  uninstall / u : do uninstall [ just for safety ]\n");
	printf("  verbose / v   : just print, do not remove\n");
	printf("  help / h      : this text\n");
	printf(" example;\n");
	printf("  %s -u\n", pname);
	printf("  %s -u -v\n", pname);
}

int main (int argc, char *argv[])
{
	int c;
	static const char *sopt = "uvh";
	static const struct option lopt[] = {
		{ "uninstall", no_argument, NULL, 'u'},
		{ "verbose", no_argument, NULL, 'v' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL,  0  }
	};

	char **f;

	while ((c = getopt_long(argc, argv, sopt, lopt, NULL)) != -1) {
		switch (c) {
			case 'u':
				uninstall = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'h':
				print_help(argv[0]);
				return 0;
		}
	}

	if (uninstall == 0) {
		print_help(argv[0]);
		return -1;
	}

	printf("uninstalling fuse-ext2\n");
	for (f = files; *f != NULL; f++) {
		if (rm_path(*f) != 0) {
			debugf("rm_path() for '%s' failed", *f);
		}
	}
	printf("done\n");
	return 0;
}
