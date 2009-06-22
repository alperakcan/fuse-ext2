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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <Carbon/Carbon.h>

static NSString *kinstalledPath = @"/Library/Filesystems/fuse-ext2.fs/Contents/Info.plist";

@interface Installer: NSObject
{

}

- (NSString *) installedVersion;
- (NSString *) availableVersion;

@end

@implementation Installer

- (NSString *) installedVersion
{
	NSString *fuse_ext2Path;
	NSString *versionString;
	NSString *bundleVersion;
	NSDictionary *fuse_ext2Plist;

	versionString = nil;

	fuse_ext2Path = kinstalledPath;
	fuse_ext2Plist = [NSDictionary dictionaryWithContentsOfFile:fuse_ext2Path];
	if (fuse_ext2Plist == nil) {
		fuse_ext2Path = [@"/System" stringByAppendingPathComponent:fuse_ext2Path];
		fuse_ext2Plist = [NSDictionary dictionaryWithContentsOfFile:fuse_ext2Path];
	}
	if (fuse_ext2Plist != nil) {
		bundleVersion = [fuse_ext2Plist objectForKey:@"CFBundleVersion"];
		if (bundleVersion != nil) {
			versionString = [[NSString alloc] initWithString:bundleVersion];
		}
	}

	return versionString;
}

- (NSString *) availableVersion
{
	return nil;
}

@end

static void print_help (const char *pname)
{
	printf("%s usage;\n", pname);
	printf("  installed / i : print installed version\n");
	printf("  available / a : print available version\n");
	printf("  url / u       : available version file url\n");
	printf("  help / h      : this text\n");
}
int main (int argc, char *argv[])
{
	int c;
	int ret;
	char *version_url;
	int list_installed;
	int list_available;

	NSString *installed;
	NSString *available;
	Installer *installer;

	NSAutoreleasePool *pool;

	static const char *sopt = "iau:h";
	static const struct option lopt[] = {
		{ "installed", no_argument, NULL, 'i' },
		{ "available", no_argument, NULL, 'a' },
		{ "url", required_argument, NULL, 'u' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL,  0  }
	};

	ret = 0;
	pool = nil;
	installer = nil;
	installed = nil;
	available = nil;

	version_url = NULL;
	list_installed = 0;
	list_available = 0;

	while ((c = getopt_long(argc, argv, sopt, lopt, NULL)) != -1) {
		switch (c) {
			case 'i':
				list_installed = 1;
				break;
			case 'a':
				list_available = 1;
				break;
			case 'u':
				version_url = optarg;
				break;
			default:
				ret = -1;
			case 'h':
				print_help(argv[0]);
				goto out;
				break;
		}
	}

	pool = [[NSAutoreleasePool alloc] init];
	if (pool == nil) {
		ret = -2;
		goto out;
	}

	installer = [[Installer alloc] init];
	if (installer == nil) {
		ret = -3;
		goto out;
	}

	if (list_installed == 1) {
		installed = [installer installedVersion];
		if (installed != nil) {
			printf("Installed Version: %s\n", [installed UTF8String]);
		} else {
			ret = -4;
		}
		goto out;
	} else if (list_available == 1) {
		installer = [[Installer alloc] init];
		available = [installer availableVersion];
		if (available != nil) {
			printf("Available Version: %s\n", [available UTF8String]);
		} else {
			ret = -5;
		}
		goto out;
	}

out:
	if (installed != nil) {
		[installed release];
	}
	if (available != nil) {
		[available release];
	}
	if (installer != nil) {
		[installer release];
	}
	if (pool != nil) {
		[pool drain];
	}
	return ret;
}
