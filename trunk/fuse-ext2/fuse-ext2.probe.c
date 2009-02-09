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

struct options_s opts;
struct private_s priv;

static const char *HOME = "http://sourceforge.net/projects/fuse-ext2/";

static const char *usage_msg =
"\n"
"%s %s %d - Probe EXT2FS volume mountability\n"
"\n"
"Copyright (C) 2008-2009 Alper Akcan <alper.akcan@gmail.com>\n"
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

static int parse_options (int argc, char *argv[])
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
				if (!opts.device) {
					opts.device = malloc(PATH_MAX + 1);
					if (!opts.device)
						return -1;

					/* We don't want relative path in /etc/mtab. */
					if (optarg[0] != '/') {
						if (!realpath(optarg, opts.device)) {
							debugf("Cannot mount %s", optarg);
							free(opts.device);
							opts.device = NULL;
							return -1;
						}
					} else
						strcpy(opts.device, optarg);
				} else {
					debugf("You must specify exactly one device");
					return -1;
				}
				break;
			case 'r':
				opts.readonly = 1;
				break;
			case 'w':
				opts.readonly = 0;
				break;
			case 'd':
				opts.debug = 1;
				break;
			default:
				debugf("Unknown option '%s'", argv[optind - 1]);
				return -1;
		}
	}

	if (!opts.device) {
		debugf("No device is specified");
		return -1;
	}

	return 0;
}

int main (int argc, char *argv[])
{
	int err = 0;
	struct stat sbuf;

	memset(&opts, 0, sizeof(opts));
	memset(&priv, 0, sizeof(priv));

	if (parse_options(argc, argv)) {
		usage();
		return -1;
	}

	if (stat(opts.device, &sbuf)) {
		debugf("Failed to access '%s'", opts.device);
		err = -3;
		goto err_out;
	}

	debugf("opts.device: %s", opts.device);

	if (do_probe() != 0) {
		debugf("Probe failed");
		err = -4;
		goto err_out;
	}

err_out:
	free(opts.device);
	return err;
}
