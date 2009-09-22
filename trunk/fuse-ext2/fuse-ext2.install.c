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
static NSString *kdownloadFilePath = @"/var/tmp/fuse-ext2.dmg";

@interface Installer: NSObject
{
	BOOL isDownloading;
	BOOL isDownloaded;
	unsigned bytesReceived;
	NSURLResponse *downloadResponse;
}

- (NSString *) installedVersion;
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
	NSLog(@"Download failed! Error - %@ %@",
		[error localizedDescription],
		[[error userInfo] objectForKey:NSErrorFailingURLStringKey]);
}

- (void) downloadDidFinish: (NSURLDownload *) download
{
	[download release];
	isDownloading = NO;
	isDownloaded = YES;
	NSLog(@"%@",@"downloadDidFinish");
}

- (void) download: (NSURLDownload *) download didReceiveResponse: (NSURLResponse *) response
{
	NSLog(@"connected to server");
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
		NSLog(@"percent complete: %f", percentComplete);
	} else {
		NSLog(@"bytes received: %d", bytesReceived);
	}
}

- (int) downloadFileFromUrl: (NSString *) urlString toFile: (NSString *) toFile
{
	NSURLRequest *urlRequest;
	NSURLDownload *urlDownload;
	NSLog(@"downloading from:%@, to:%@", urlString, toFile);
	urlRequest = [NSURLRequest requestWithURL:[NSURL URLWithString:urlString]
	                           cachePolicy:NSURLRequestUseProtocolCachePolicy
	                           timeoutInterval:60.0];
	urlDownload = [[NSURLDownload alloc] initWithRequest:urlRequest delegate:self];
	if (urlDownload == nil) {
		NSLog(@"NSURLDownload[] failed");
		return -1;
	}
	[urlDownload setDestination:toFile allowOverwrite:YES];
	NSLog(@"downloading started");
	isDownloading = YES;
	isDownloaded = NO;
	while (isDownloading == YES) {
		[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0]];
	}
	if (isDownloaded == NO) {
		NSLog(@"downloading failed");
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
	NSLog(@"checking from '%@'", urlString);
	url = [NSURL URLWithString:urlString];
	request = [NSURLRequest requestWithURL:url cachePolicy:NSURLRequestReturnCacheDataElseLoad timeoutInterval:30.0];
	data = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error];
	if (data == nil) {
		NSLog(@"NSURLConnection failed (%@)", error);
		return nil;
	}
	doc = [[NSXMLDocument alloc] initWithData:data options:0 error:&error];
	if (doc == nil) {
		NSLog(@"NSXMLDocument failed (%@)", error);
		return nil;
	}
	NSLog(@"doc=%@", doc);
	nodes = [doc nodesForXPath:valueName error:&error];
	if (nodes == nil || [nodes count] == 0) {
		[doc release];
		NSLog(@"nodesForXPath:fuse-ext2 failed (%@)", error);
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

- (int) updateVersion: (NSString *) urlString
{
	int ret;
	NSString *md5sum;
	NSString *version;
	NSString *location;
	md5sum = [self getValueFromUrl: urlString valueName:kversionMd5sum];
	version = [self getValueFromUrl: urlString valueName:kversionVersion];
	location = [self getValueFromUrl: urlString valueName:kversionLocation];
	if (md5sum == nil || version == nil || location == nil) {
		NSLog(@"could not get version, location, or md5sum");
		goto out;
	}
	NSLog(@"updating from version: %@, location: %@, md5sum: %@", version, location, md5sum);
	ret = [self downloadFileFromUrl:location toFile:kdownloadFilePath];
	if (ret != 0) {
		NSLog(@"downloading file failed");
		goto out;
	}
	return 0;
out:
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
	printf("  %s -i\n");
	printf("  %s -a -u url\n");
}

int main (int argc, char *argv[])
{
	int c;
	int ret;
	int update;
	char *version_url;
	int list_installed;
	int list_available;

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
			NSLog(@"Installed Version: %@", installed);
		} else {
			ret = -4;
		}
		goto out;
	} else if (list_available == 1) {
		if (version_url == NULL) {
			NSLog(@"missing url variable");
			ret = -5;
			goto out;
		}
		available = [installer availableVersion:[NSString stringWithFormat:@"%s", version_url]];
		if (available != nil) {
			NSLog(@"Available Version: %@", available);
		} else {
			ret = -6;
		}
		goto out;
	} else if (update == 1) {
		if (version_url == NULL) {
			NSLog(@"missing url variable");
			ret = -7;
			goto out;
		}
		ret = [installer updateVersion:[NSString stringWithFormat:@"%s", version_url]];
		if (ret != 0) {
			NSLog(@"updateVersion failed");
			ret = -8;
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
