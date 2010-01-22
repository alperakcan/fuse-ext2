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

static const char *HOME = "http://sourceforge.net/projects/fuse-ext2/";

static const char *usage_msg =
"\n"
"%s %s %d - Probe EXT2FS volume mountability\n"
"\n"
"Copyright (C) 2008-2010 Alper Akcan <alper.akcan@gmail.com>\n"
"\n"
"Usage:    %s <--readonly|--readwrite> <device|image_file>\n"
"\n"
"Example:  fuse-ext2.probe --readwrite /dev/sda1\n"
"\n"
"%s\n"
"\n";

static void usage (void)
{
	printf(usage_msg, PACKAGE, VERSION, fuse_version(), PACKAGE, HOME);
}

static int parse_options (int argc, char *argv[], struct extfs_data *opts)
{
	int c;

	static const char *sopt = "-rwd";
	static const struct option lopt[] = {
		{ "readonly",	 no_argument,	NULL, 'r' },
		{ "readwrite",	 no_argument,	NULL, 'w' },
		{ "debug",       no_argument,   NULL, 'd' },
		{ NULL,		 0,		NULL,  0  }
	};

	opterr = 0; /* We'll handle the errors, thank you. */

	while ((c = getopt_long(argc, argv, sopt, lopt, NULL)) != -1) {
		switch (c) {
			case 1:	/* A non-option argument */
				if (!opts->device) {
					opts->device = malloc(PATH_MAX + 1);
					if (!opts->device)
						return -1;

					/* We don't want relative path in /etc/mtab. */
					if (optarg[0] != '/') {
						if (!realpath(optarg, opts->device)) {
							debugf_main("Cannot mount %s", optarg);
							free(opts->device);
							opts->device = NULL;
							return -1;
						}
					} else
						strcpy(opts->device, optarg);
				} else {
					debugf_main("You must specify exactly one device");
					return -1;
				}
				break;
			case 'r':
				opts->readonly = 1;
				break;
			case 'w':
				opts->readonly = 0;
				break;
			case 'd':
				opts->debug = 1;
				break;
			default:
				debugf_main("Unknown option '%s'", argv[optind - 1]);
				return -1;
		}
	}

	if (!opts->device) {
		debugf_main("No device is specified");
		return -1;
	}

	return 0;
}

int main (int argc, char *argv[])
{
	int err = 0;
	struct stat sbuf;
	struct extfs_data opts;

	memset(&opts, 0, sizeof(opts));

	if (parse_options(argc, argv, &opts)) {
		usage();
		return -1;
	}

	if (stat(opts.device, &sbuf)) {
		debugf_main("Failed to access '%s'", opts.device);
		err = -3;
		goto err_out;
	}

	debugf_main("opts.device: %s", opts.device);

	if (do_probe(&opts) != 0) {
		debugf_main("Probe failed");
		err = -4;
		goto err_out;
	}

err_out:
	free(opts.device);
	free(opts.volname);
	free(opts.options);
	return err;
}
