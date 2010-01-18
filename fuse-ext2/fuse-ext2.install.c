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

/*
 * example version xml
 *
 * <release>
 *   <fuse-ext2>
 *     <version>version</version>
 *     <location>location</location>
 *     <md5sum>md5sum</md5sum>
 *   </fuse-ext2>
 *   <fuse-ext2>
 *     <version>version</version>
 *     <location>location</location>
 *     <md5sum>md5sum</md5sum>
 *   </fuse-ext2>
 * </release>
 *
 * <release>
 *   <fuse-ext2>
 *     <version>0.0.6</version>
 *     <location>http://prdownloads.sourceforge.net/fuse-ext2/fuse-ext2-0.0.6.dmg?download</location>
 *     <md5sum>md5sum</md5sum>
 *   </fuse-ext2>
 *   <fuse-ext2>
 *     <version>0.0.5</version>
 *     <location>http://prdownloads.sourceforge.net/fuse-ext2/fuse-ext2-0.0.5.dmg?download</location>
 *     <md5sum>md5sum</md5sum>
 *   </fuse-ext2>
 * </release>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include <Foundation/Foundation.h>

static NSString *kinstalledPath = @"/Library/Filesystems/fuse-ext2.fs/Contents/Info.plist";
static NSString *kversionVersion = @"release/fuse-ext2/version";
static NSString *kversionLocation = @"release/fuse-ext2/location";
static NSString *kversionMd5sum = @"release/fuse-ext2/md5sum";
static NSString *ktempPath = @"/var/tmp/fuse-ext2.tmp";
static NSString *kmountPath = @"/var/tmp/fuse-ext2.tmp/mnt";
static NSString *kpkgPath = @"/var/tmp/fuse-ext2.tmp/mnt/fuse-ext2.pkg";
static NSString *kdownloadFilePath = @"/var/tmp/fuse-ext2.tmp/fuse-ext2.dmg";
static NSString *kdefaultUrl = @"http://fuse-ext2.svn.sourceforge.net/viewvc/fuse-ext2/release/release.xml";
static const NSTimeInterval kNetworkTimeOutInterval = 60.00;

@interface Installer: NSObject
{
	BOOL isDownloading;
	BOOL isDownloaded;
	unsigned bytesReceived;
	NSURLResponse *downloadResponse;
}

- (NSString *) installedVersion;
- (int) runTaskForPath: (NSString *) path withArguments: (NSArray *) args output: (NSData **) output;
- (int) downloadFileFromUrl: (NSString *) urlString toFile: (NSString *) toFile;
- (NSString *) getValueFromUrl: (NSString *) urlString valueName: (NSString *) valueName;
- (NSString *) availableVersion: (NSString *) urlString;
- (int) updateVersion: (NSString *) urlString;

- (void) download: (NSURLDownload *) download didFailWithError: (NSError *) error;
- (void) downloadDidFinish: (NSURLDownload *) download;
- (void) download: (NSURLDownload *) download didReceiveResponse: (NSURLResponse *) response;
- (void) download: (NSURLDownload *) download didReceiveDataOfLength: (unsigned) length;

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
		bundleVersion = [fuse_ext2Plist objectForKey:(NSString *) kCFBundleVersionKey];
		if (bundleVersion != nil) {
			versionString = [[NSString alloc] initWithString:bundleVersion];
		}
	}
	return versionString;
}

- (void) download: (NSURLDownload *) download didFailWithError: (NSError *) error
{
	[download release];
	isDownloading = NO;
	isDownloaded = NO;
	NSLog(@"fuse-ext2.install: download failed! error - %@ %@",
		[error localizedDescription],
		[[error userInfo] objectForKey:NSErrorFailingURLStringKey]);
}

- (void) downloadDidFinish: (NSURLDownload *) download
{
	[download release];
	isDownloading = NO;
	isDownloaded = YES;
	NSLog(@"fuse-ext2.install: download finished");
}

- (void) download: (NSURLDownload *) download didReceiveResponse: (NSURLResponse *) response
{
	NSLog(@"fuse-ext2.install: connected to server");
	bytesReceived = 0;
	[response retain];
	[downloadResponse release];
	downloadResponse = response;
}

- (void) download: (NSURLDownload *) download didReceiveDataOfLength: (unsigned) length
{
	long long expectedSize;
	expectedSize = [downloadResponse expectedContentLength];
	bytesReceived += length;
	if (expectedSize != NSURLResponseUnknownLength) {
		float percentComplete;
		percentComplete = (bytesReceived / (float) expectedSize) * 100.00;
		NSLog(@"fuse-ext2.install: percent complete: %f", percentComplete);
	} else {
		NSLog(@"fuse-ext2.install: bytes received: %d", bytesReceived);
	}
}

- (int) downloadFileFromUrl: (NSString *) urlString toFile: (NSString *) toFile
{
	NSURLRequest *urlRequest;
	NSURLDownload *urlDownload;
	NSLog(@"fuse-ext2.install: downloading from:%@, to:%@", urlString, toFile);
	urlRequest = [NSURLRequest requestWithURL:[NSURL URLWithString:urlString]
	                           cachePolicy:NSURLRequestUseProtocolCachePolicy
	                           timeoutInterval:kNetworkTimeOutInterval];
	urlDownload = [[NSURLDownload alloc] initWithRequest:urlRequest delegate:self];
	if (urlDownload == nil) {
		NSLog(@"fuse-ext2.install: NSURLDownload[] failed");
		return -1;
	}
	[urlDownload setDestination:toFile allowOverwrite:YES];
	NSLog(@"fuse-ext2.install: downloading started");
	isDownloading = YES;
	isDownloaded = NO;
	while (isDownloading == YES) {
		[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0]];
	}
	if (isDownloaded == NO) {
		NSLog(@"fuse-ext2.install: downloading failed");
		return -1;
	}
	return 0;
}

- (NSString *) getValueFromUrl: (NSString *) urlString valueName: (NSString *) valueName
{
	NSURL *url;
	NSData *data;
	NSArray *node;
	NSArray *nodes;
	NSError *error;
	NSString *value;
	NSXMLDocument *doc;
	NSURLRequest *request;
	NSURLResponse *response;
	NSLog(@"fuse-ext2.install: checking from '%@'", urlString);
	url = [NSURL URLWithString:urlString];
	request = [NSURLRequest requestWithURL:url cachePolicy:NSURLRequestReturnCacheDataElseLoad timeoutInterval:30.0];
	data = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error];
	if (data == nil) {
		NSLog(@"fuse-ext2.install: NSURLConnection failed (%@)", error);
		return nil;
	}
	doc = [[NSXMLDocument alloc] initWithData:data options:0 error:&error];
	if (doc == nil) {
		NSLog(@"fuse-ext2.install: NSXMLDocument failed (%@)", error);
		return nil;
	}
	NSLog(@"fuse-ext2.install: result;\n'%@'", doc);
	nodes = [doc nodesForXPath:valueName error:&error];
	if (nodes == nil || [nodes count] == 0) {
		[doc release];
		NSLog(@"fuse-ext2.install: nodesForXPath:fuse-ext2 failed (%@)", error);
		return nil;
	}
	value = [[NSString alloc] initWithString:[[nodes objectAtIndex:0] stringValue]];
	[doc release];
	return value;
}

- (NSString *) availableVersion: (NSString *) urlString
{
	return [self getValueFromUrl: urlString valueName:kversionVersion];
}

- (int) runTaskForPath: (NSString *) path withArguments: (NSArray *) args output: (NSData **) output
{
	int ret;
	NSTask *task;
	NSPipe *pipe;
	NSDate *date;
	NSRunLoop *loop;
	NSFileHandle *outFile;
	task = [[[NSTask alloc] init] autorelease];
	[task setLaunchPath:path];
	[task setArguments:args];
	[task setEnvironment:[NSDictionary dictionary]];
	pipe = [NSPipe pipe];
	[task setStandardOutput:pipe];
	@try {
		[task launch];
	} @catch (NSException *err) {
		NSLog(@"fuse-ext2.install: caught exception %@ when launching task %@", err, task);
		return -1;
	}
	ret = 0;
	outFile = nil;
	loop = [NSRunLoop currentRunLoop];
	date = [NSDate date];
	do {
		NSDate *waitDate = [NSDate dateWithTimeIntervalSinceNow:0.01];
		if ([waitDate timeIntervalSinceDate:date] > kNetworkTimeOutInterval) {
			ret = -1;
			[task terminate];
		}
		[loop runUntilDate:waitDate];
	} while ([task isRunning]);

	if (ret == 0) {
		ret = [task terminationStatus];
	}
	if (output) {
		outFile = [pipe fileHandleForReading];
	}
	if (outFile && output) {
		*output = [outFile readDataToEndOfFile];
	}
	return ret;
}

- (int) updateVersion: (NSString *) urlString
{
	int ret;
	NSString *md5sum;
	NSString *version;
	NSString *location;
	ret = [self runTaskForPath:@"/bin/mkdir" withArguments:[NSArray arrayWithObjects:@"-p", ktempPath, nil] output:nil];
	if (ret != 0) {
		NSLog(@"fuse-ext2.install: /bin/mkdir failed");
		goto out;
	}
	ret = [self runTaskForPath:@"/bin/mkdir" withArguments:[NSArray arrayWithObjects:@"-p", kmountPath, nil] output:nil];
	if (ret != 0) {
		NSLog(@"fuse-ext2.install: /bin/mkdir failed");
		goto out;
	}
	md5sum = [self getValueFromUrl: urlString valueName:kversionMd5sum];
	version = [self getValueFromUrl: urlString valueName:kversionVersion];
	location = [self getValueFromUrl: urlString valueName:kversionLocation];
	if (md5sum == nil || version == nil || location == nil) {
		NSLog(@"fuse-ext2.install: could not get version, location, or md5sum");
		goto out;
	}
	NSLog(@"fuse-ext2.install: updating from version: %@, location: %@, md5sum: %@", version, location, md5sum);
	ret = [self downloadFileFromUrl:location toFile:kdownloadFilePath];
	if (ret != 0) {
		NSLog(@"fuse-ext2.install: downloading file failed");
		goto out;
	}
	/*
	 * hdiutil attach -private -nobrowse -mountpoint /var/tmp/fuse-ext2.tmp/mnt /var/tmp/fuse-ext2.tmp/fuse-ext2.dmg
	 * hdiutil detach /var/tmp/fuse-ext2.tmp/mnt
	 */
	ret = [self runTaskForPath:@"/usr/bin/hdiutil"
	            withArguments:[NSArray arrayWithObjects:@"attach",
	                                                    @"-private",
	                                                    @"-nobrowse",
	                                                    @"-mountpoint",
	                                                    kmountPath,
	                                                    kdownloadFilePath,
	                                                    nil]
	            output:nil];
	if (ret != 0) {
		NSLog(@"fuse-ext2.install: hdiutil attach failed");
		goto out;
	}
	ret = [self runTaskForPath:@"/usr/local/bin/fuse-ext2.uninstall" withArguments:[NSArray arrayWithObjects:nil] output:nil];
	/*
	 * sudo installer -pkg /Volumes/fuse-ext2/fuse-ext2.pkg -target / -verbose -dumplog
	 */
	ret = [self runTaskForPath:@"/usr/sbin/installer"
	            withArguments:[NSArray arrayWithObjects:@"-pkg",
							    kpkgPath,
							    @"-target",
							    @"/",
							    @"-verbose",
							    @"-dumplog",
							    nil]
		    output:nil];
	if (ret != 0) {
		NSLog(@"fuse-ext2.install: installer failed");
		goto out;
	}
	[self runTaskForPath:@"/usr/bin/hdiutil" withArguments:[NSArray arrayWithObjects:@"detach", kmountPath, nil] output:nil];
	[self runTaskForPath:@"/bin/rm" withArguments:[NSArray arrayWithObjects:@"-rf", ktempPath, nil] output:nil];
	[md5sum release];
	[version release];
	[location release];
	return 0;
out:
	[self runTaskForPath:@"/usr/bin/hdiutil" withArguments:[NSArray arrayWithObjects:@"detach", kmountPath, nil] output:nil];
	[self runTaskForPath:@"/bin/rm" withArguments:[NSArray arrayWithObjects:@"-rf", ktempPath, nil] output:nil];
	[md5sum release];
	[version release];
	[location release];
	return -1;
}

@end

static void print_help (const char *pname)
{
	printf("%s usage;\n", pname);
	printf("  installed / i : print installed version\n");
	printf("  available / a : print available version\n");
	printf("  update / r    : update version\n");
	printf("  url / u       : available version file url\n");
	printf("  help / h      : this text\n");
	printf(" example;\n");
	printf("  %s -i\n", pname);
	printf("  %s -a -u url\n", pname);
}

int main (int argc, char *argv[])
{
	int c;
	int ret;
	int update;
	int list_installed;
	int list_available;
	const char *version_url;

	NSString *installed;
	NSString *available;
	Installer *installer;

	NSAutoreleasePool *pool;

	static const char *sopt = "iaru:h";
	static const struct option lopt[] = {
		{ "installed", no_argument, NULL, 'i' },
		{ "available", no_argument, NULL, 'a' },
		{ "update", no_argument, NULL, 'r' },
		{ "url", required_argument, NULL, 'u' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL,  0  }
	};

	ret = 0;
	pool = nil;
	installer = nil;
	installed = nil;
	available = nil;

	update = 0;
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
			case 'r':
				update = 1;
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
			printf("Installed Version:%s\n", [installed UTF8String]);
			ret = 0;
		} else {
			ret = -4;
		}
		goto out;
	} else if (list_available == 1) {
		if (version_url == NULL) {
			NSLog(@"fuse-ext2.install: using default url:%@", kdefaultUrl);
			version_url = [kdefaultUrl UTF8String];
		}
		available = [installer availableVersion:[NSString stringWithFormat:@"%s", version_url]];
		if (available != nil) {
			printf("Available Version:%s\n", [available UTF8String]);
			ret = 0;
		} else {
			ret = -6;
		}
		goto out;
	} else if (update == 1) {
		if (version_url == NULL) {
			NSLog(@"fuse-ext2.install: using default url:%@", kdefaultUrl);
			version_url = [kdefaultUrl UTF8String];
		}
		ret = [installer updateVersion:[NSString stringWithFormat:@"%s", version_url]];
		if (ret != 0) {
			NSLog(@"fuse-ext2.install: updateVersion failed");
			ret = -8;
		}
		goto out;
	} else {
		print_help(argv[0]);
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
