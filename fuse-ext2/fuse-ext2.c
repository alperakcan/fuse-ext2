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

#if __FreeBSD__ == 10
static char def_opts[] = "allow_other,local,";
static char def_opts_rd[] = "noappledouble,";
#else
static char def_opts[] = "";
static char def_opts_rd[] = "";
#endif

static const char *usage_msg =
"\n"
"%s %s %d - FUSE EXT2FS Driver\n"
"\n"
"Copyright (C) 2008-2010 Alper Akcan <alper.akcan@gmail.com>\n"
"Copyright (C) 2009 Renzo Davoli <renzo@cs.unibo.it>\n"
"\n"
"Usage:    %s <device|image_file> <mount_point> [-o option[,...]]\n"
"\n"
"Options:  ro, force, allow_others\n"
"          Please see details in the manual.\n"
"\n"
"Example:  fuse-ext2 /dev/sda1 /mnt/sda1\n"
"\n"
"%s\n"
"\n";

static int strappend (char **dest, const char *append)
{
	char *p;
	size_t size;

	if (!dest) {
		return -1;
	}
	if (!append) {
		return 0;
	}

	size = strlen(append) + 1;
	if (*dest) {
		size += strlen(*dest);
	}

	p = realloc(*dest, size);
    	if (!p) {
    		debugf_main("Memory realloction failed");
		return -1;
	}

	if (*dest) {
		strcat(p, append);
	} else {
		strcpy(p, append);
	}
	*dest = p;

	return 0;
}

static void usage (void)
{
	printf(usage_msg, PACKAGE, VERSION, fuse_version(), PACKAGE, HOME);
}

static int parse_options (int argc, char *argv[], struct extfs_data *opts)
{
	int c;

	static const char *sopt = "o:hv";
	static const struct option lopt[] = {
		{ "options",	 required_argument,	NULL, 'o' },
		{ "help",	 no_argument,		NULL, 'h' },
		{ "verbose",	 no_argument,		NULL, 'v' },
		{ NULL,		 0,			NULL,  0  }
	};

#if 0
	printf("arguments;\n");
	for (c = 0; c < argc; c++) {
		printf("%d: %s\n", c, argv[c]);
	}
	printf("done\n");
#endif

	opterr = 0; /* We'll handle the errors, thank you. */

	while ((c = getopt_long(argc, argv, sopt, lopt, NULL)) != -1) {
		switch (c) {
			case 'o':
				if (opts->options)
					if (strappend(&opts->options, ","))
						return -1;
				if (strappend(&opts->options, optarg))
					return -1;
				break;
			case 'h':
				usage();
				exit(9);
			case 'v':
				/*
				 * We must handle the 'verbose' option even if
				 * we don't use it because mount(8) passes it.
				 */
				opts->debug = 1;
				break;
			default:
				debugf_main("Unknown option '%s'", argv[optind - 1]);
				return -1;
		}
	}

	if (optind < argc) {
		optarg=argv[optind++];
		if (optarg[0] != '/') {
			char fulldevice[PATH_MAX+1];
			if (!realpath(optarg, fulldevice)) {
				debugf_main("Cannot mount %s", optarg);
				free(opts->device);
				opts->device = NULL;
				return -1;
			} else
				opts->device = strdup(fulldevice);
		} else
			opts->device = strdup(optarg);
	}

	if (optind < argc) {
		opts->mnt_point = argv[optind++];
	}

	if (optind < argc) {
		debugf_main("You must specify exactly one device and exactly one mount point");
		return -1;
	}

	if (!opts->device) {
		debugf_main("No device is specified");
		return -1;
	}
	if (!opts->mnt_point) {
		debugf_main("No mountpoint is specified");
		return -1;
	}

	return 0;
}

static char * parse_mount_options (const char *orig_opts, struct extfs_data *opts)
{
	char *options, *s, *opt, *val, *ret;

	ret = malloc(strlen(def_opts) + strlen(def_opts_rd) + strlen(orig_opts) + 256 + PATH_MAX);
	if (!ret) {
		return NULL;
	}

	*ret = 0;
	options = strdup(orig_opts);
	if (!options) {
		debugf_main("strdup failed");
		return NULL;
	}

	s = options;
	while (s && *s && (val = strsep(&s, ","))) {
		opt = strsep(&val, "=");
		if (!strcmp(opt, "ro")) { /* Read-only mount. */
			if (val) {
				debugf_main("'ro' option should not have value");
				goto err_exit;
			}
			opts->readonly = 1;
			strcat(ret, "ro,");
		} else if (!strcmp(opt, "rw")) { /* Read-write mount */
			if (val) {
				debugf_main("'rw' option should not have value");
				goto err_exit;
			}
			opts->readonly = 0;
			strcat(ret, "rw,");
		} else if (!strcmp(opt, "rw+")) { /* Read-write mount */
			if (val) {
				debugf_main("'rw+' option should not have value");
				goto err_exit;
			}
			opts->readonly = 0;
			opts->force = 1;
			strcat(ret, "rw,");
		} else if (!strcmp(opt, "debug")) { /* enable debug */
			if (val) {
				debugf_main("'debug' option should not have value");
				goto err_exit;
			}
			opts->debug = 1;
			strcat(ret, "debug,");
		} else if (!strcmp(opt, "silent")) { /* keep silent */
			if (val) {
				debugf_main("'silent' option should not have value");
				goto err_exit;
			}
			opts->silent = 1;
		} else if (!strcmp(opt, "force")) { /* enable read/write */
			if (val) {
				debugf_main("'force option should no have value");
				goto err_exit;
			}
			opts->force = 1;
#if __FreeBSD__ == 10
			strcat(ret, "force,");
#endif
		} else { /* Probably FUSE option. */
			strcat(ret, opt);
			if (val) {
				strcat(ret, "=");
				strcat(ret, val);
			}
			strcat(ret, ",");
		}
	}

	if (opts->readonly == 0 && opts->force == 0) {
		fprintf(stderr, "Mounting %s Read-Only.\nUse \'force\' or \'rw+\' options to enable Read-Write mode\n",opts->device);
		opts->readonly = 1;
	}

	strcat(ret, def_opts);
	if (opts->readonly == 1) {
		strcat(ret, def_opts_rd);
		strcat(ret, "ro,");
	}
	strcat(ret, "fsname=");
	strcat(ret, opts->device);
#if __FreeBSD__ == 10
	strcat(ret, ",fstypename=");
	strcat(ret, "ext2");
	strcat(ret, ",volname=");
	if (opts->volname == NULL || opts->volname[0] == '\0') {
		s = strrchr(opts->device, '/');
		if (s != NULL) {
			strcat(ret, s + 1);
		} else {
			strcat(ret, opts->device);
		}
	} else {
		strcat(ret, opts->volname);
	}
#endif
exit:
	free(options);
	return ret;
err_exit:
	free(ret);
	ret = NULL;
	goto exit;
}

static const struct fuse_operations ext2fs_ops = {
	.getattr        = op_getattr,
	.readlink       = op_readlink,
	.mknod          = op_mknod,
	.mkdir          = op_mkdir,
	.unlink         = op_unlink,
	.rmdir          = op_rmdir,
	.symlink        = op_symlink,
	.rename         = op_rename,
	.link           = op_link,
	.chmod          = op_chmod,
	.chown          = op_chown,
	.truncate       = op_truncate,
	.open           = op_open,
	.read           = op_read,
	.write          = op_write,
	.statfs         = op_statfs,
	.flush          = op_flush,
	.release	= op_release,
	.fsync          = op_fsync,
	.setxattr       = NULL,
	.getxattr       = NULL,
	.listxattr      = NULL,
	.removexattr    = NULL,
	.opendir        = op_open,
	.readdir        = op_readdir,
	.releasedir     = op_release,
	.fsyncdir       = op_fsync,
	.init		= op_init,
	.destroy	= op_destroy,
	.access         = op_access,
	.create         = op_create,
	.ftruncate      = op_ftruncate,
	.fgetattr       = op_fgetattr,
	.lock           = NULL,
	.utimens        = op_utimens,
	.bmap           = NULL,
};

int main (int argc, char *argv[])
{
	int err = 0;
	struct stat sbuf;
	char *parsed_options = NULL;
	struct fuse_args fargs = FUSE_ARGS_INIT(0, NULL);
	struct extfs_data opts;

	debugf_main("version:'%s', fuse_version:'%d / %d / %d'", VERSION, FUSE_USE_VERSION,  FUSE_VERSION, fuse_version());

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

	if (do_probe(&opts) != 0) {
		debugf_main("Probe failed");
		err = -4;
		goto err_out;
	}

	parsed_options = parse_mount_options(opts.options ? opts.options : "", &opts);
	if (!parsed_options) {
		err = -2;
		goto err_out;
	}

	debugf_main("opts.device: %s", opts.device);
	debugf_main("opts.mnt_point: %s", opts.mnt_point);
	debugf_main("opts.volname: %s", (opts.volname != NULL) ? opts.volname : "");
	debugf_main("opts.options: %s", opts.options);
	debugf_main("parsed_options: %s", parsed_options);

	if (fuse_opt_add_arg(&fargs, PACKAGE) == -1 ||
	    fuse_opt_add_arg(&fargs, "-s") == -1 ||
	    fuse_opt_add_arg(&fargs, "-o") == -1 ||
	    fuse_opt_add_arg(&fargs, parsed_options) == -1 ||
	    fuse_opt_add_arg(&fargs, opts.mnt_point) == -1) {
		debugf_main("Failed to set FUSE options");
		fuse_opt_free_args(&fargs);
		err = -5;
		goto err_out;
	}

	if (opts.readonly == 0) {
		debugf_main("mounting read-write");
	} else {
		debugf_main("mounting read-only");
	}

	fuse_main(fargs.argc, fargs.argv, &ext2fs_ops, &opts);

err_out:
	fuse_opt_free_args(&fargs);
	free(parsed_options);
	free(opts.options);
	free(opts.device);
	free(opts.volname);
	return err;
}
