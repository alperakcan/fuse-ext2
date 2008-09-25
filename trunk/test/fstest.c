/* filesystem verification tool, designed to detect data corruption on a filesystem

   tridge@samba.org, March 2002
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

/* variables settable on the command line */
static int loop_count = 100;
static int num_files = 1;
static int file_size = 1024*1024;
static int block_size = 1024;
static char *base_dir = ".";
static int use_mmap;
static int use_sync;

typedef unsigned char uchar;

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

static void *x_malloc(int size)
{
	void *ret = malloc(size);
	if (!ret) {
		fprintf(stderr,"Out of memory for size %d!\n", size);
		exit(1);
	}
	return ret;
}


/* generate a buffer for a particular child, fnum etc. Just use a simple buffer
   to make debugging easy 
*/
static void gen_buffer(uchar *buf, int loop, int child, int fnum, int ofs)
{
	uchar v = (loop+child+fnum+(ofs/block_size)) % 256;
	memset(buf, v, block_size);
}

/* 
   check if a buffer from disk is correct
*/
static void check_buffer(uchar *buf, int loop, int child, int fnum, int ofs)
{
	uchar *buf2;

	buf2 = x_malloc(block_size);

	gen_buffer(buf2, loop, child, fnum, ofs);
	
	if (memcmp(buf, buf2, block_size) != 0) {
		int i, j;
		for (i=0;buf[i] == buf2[i] && i<block_size;i++) ;
		fprintf(stderr,"Corruption in child %d fnum %d at offset %d\n",
			child, fnum, ofs+i);

		printf("Correct:   ");
		for (j=0;j<MIN(20, block_size-i);j++) {
			printf("%02x ", buf2[j+i]);
		}
		printf("\n");

		printf("Incorrect: ");
		for (j=0;j<MIN(20, block_size-i);j++) {
			printf("%02x ", buf[j+i]);
		}
		printf("\n");
		exit(1);
	}

	free(buf2);
}

/*
  create a file with a known data set for a child
 */
static void create_file(const char *dir, int loop, int child, int fnum)
{
	uchar *buf;
	int size, fd;
	char fname[1024];

	buf = x_malloc(block_size);
	sprintf(fname, "%s/file%d", dir, fnum);
	fd = open(fname, O_RDWR|O_CREAT|O_TRUNC | (use_sync?O_SYNC:0), 0644);
	if (fd == -1) {
		perror(fname);
		exit(1);
	}
		
	if (!use_mmap) {
		for (size=0; size<file_size; size += block_size) {
			gen_buffer(buf, loop, child, fnum, size);
			if (pwrite(fd, buf, block_size, size) != block_size) {
				fprintf(stderr,"Write failed at offset %d\n", size);
				exit(1);
			}
		}
	} else {
		char *p;
		if (ftruncate(fd, file_size) != 0) {
			perror("ftruncate");
			exit(1);
		}
		p = mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		if (p == (char *)-1) {
			perror("mmap");
			exit(1);
		}
		for (size=0; size<file_size; size += block_size) {
			gen_buffer(p+size, loop, child, fnum, size);
		}
		munmap(p, file_size);
	}

	free(buf);
	close(fd);
}

/* 
   check that a file has the right data
 */
static void check_file(const char *dir, int loop, int child, int fnum)
{
	uchar *buf;
	int size, fd;
	char fname[1024];

	buf = x_malloc(block_size);

	sprintf(fname, "%s/file%d", dir, fnum);
	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror(fname);
		exit(1);
	}

	for (size=0; size<file_size; size += block_size) {
		if (pread(fd, buf, block_size, size) != block_size) {
			fprintf(stderr,"read failed at offset %d\n", size);
			exit(1);
		}
		check_buffer(buf, loop, child, fnum, size);
	}

	free(buf);
	close(fd);
}

/* 
   revsusive directory traversal - used for cleanup
   fn() is called on all files/dirs in the tree
 */
void traverse(const char *dir, int (*fn)(const char *))
{
	DIR *d;
	struct dirent *de;

	d = opendir(dir);
	if (!d) return;

	while ((de = readdir(d))) {
		char fname[1024];
		struct stat st;

		if (strcmp(de->d_name,".") == 0) continue;
		if (strcmp(de->d_name,"..") == 0) continue;

		sprintf(fname, "%s/%s", dir, de->d_name);
		if (lstat(fname, &st)) {
			perror(fname);
			continue;
		}

		if (S_ISDIR(st.st_mode)) {
			traverse(fname, fn);
		}

		fn(fname);
	}

	closedir(d);
}

/* the main child function - this creates/checks the file for one child */
static void run_child(int child)
{
	int i, loop;
	char dir[1024];

	sprintf(dir, "%s/child%d", base_dir, child);

	/* cleanup any old files */
	if (remove(dir) != 0 && errno != ENOENT) {
		printf("Child %d cleaning %s\n", child, dir);
		traverse(dir, remove);
		remove(dir);
	}

	if (mkdir(dir, 0755) != 0) {
		perror(dir);
		exit(1);
	}

	for (loop = 0; loop < loop_count; loop++) {
		printf("Child %d loop %d\n", child, loop);
		for (i=0;i<num_files;i++) {
			create_file(dir, loop, child, i);
		}
		for (i=0;i<num_files;i++) {
			check_file(dir, loop, child, i);
		}
	}

	/* cleanup afterwards */
	printf("Child %d cleaning up %s\n", child, dir);
	traverse(dir, remove);
	remove(dir);

	exit(0);
}

static void usage(void)
{
	printf("\n"
"Usage: fstest [options]\n"
"\n"
" -n num_children       set number of child processes\n"
" -f num_files          set number of files\n"
" -s file_size          set file sizes\n"
" -b block_size         set block (IO) size\n"
" -p path               set base path\n"
" -l loops              set loop count\n"
" -m                    use mmap\n"
" -S                    use synchronous IO\n"
" -h                    show this help message\n");
}

/* main program */
int main(int argc, char *argv[])
{
	int c;
	extern char *optarg;
	extern int optind;
	int num_children = 1;
	int i, status, ret;

	while ((c = getopt(argc, argv, "n:s:f:p:l:b:Shm")) != -1) {
		switch (c) {
		case 'n':
			num_children = strtol(optarg, NULL, 0);
			break;
		case 'b':
			block_size = strtol(optarg, NULL, 0);
			break;
		case 'f':
			num_files = strtol(optarg, NULL, 0);
			break;
		case 's':
			file_size = strtol(optarg, NULL, 0);
			break;
		case 'p':
			base_dir = optarg;
			break;
		case 'm':
			use_mmap = 1;
			break;
		case 'S':
			use_sync = 1;
			break;
		case 'l':
			loop_count = strtol(optarg, NULL, 0);
			break;
		case 'h':
			usage();
			exit(0);
		default:
			usage();
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	/* round up the file size */
	if (file_size % block_size != 0) {
		file_size = (file_size + (block_size-1)) / block_size;
		file_size *= block_size;
		printf("Rounded file size to %d\n", file_size);
	}

	printf("num_children=%d file_size=%d num_files=%d loop_count=%d block_size=%d\nmmap=%d sync=%d\n",
	       num_children, file_size, num_files, loop_count, block_size, use_mmap, use_sync);

	printf("Total data size %.1f Mbyte\n",
	       num_files * num_children * 1.0e-6 * file_size);

	/* fork and run run_child() for each child */
	for (i=0;i<num_children;i++) {
		if (fork() == 0) {
			run_child(i);
			exit(0);
		}
	}

	ret = 0;

	/* wait for children to exit */
	while (waitpid(0, &status, 0) == 0 || errno != ECHILD) {
		if (WEXITSTATUS(status) != 0) {
			ret = WEXITSTATUS(status);
			printf("Child exited with status %d\n", ret);
		}
	}

	if (ret != 0) {
		printf("fstest failed with status %d\n", ret);
	}

	return ret;
}
