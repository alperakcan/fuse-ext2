//
//  fuse_ext2Pref.m
//  fuse-ext2
//
//  Created by Alper Akcan on 4/27/09.
//  Copyright (c) 2009 __MyCompanyName__. All rights reserved.
//

#import "fuse_ext2Pref.h"

static NSString *kinstalledPath = @"/Library/Filesystems/fuse-ext2.fs/Contents/Info.plist";
static NSString *kaboutLabelString = @"fuse-ext2 is a ext2/ext3 filesystem support for Fuse. Please visit fuse-ext2 homepage for more information.";
static NSString *kinstalledString = @"Installed Version:";
static NSString *kupdateString = @"No Updates Available At This Time.";

@interface fuse_ext2Pref (PrivateMethods)

- (NSString *) installedVersion;
- (void) updateGUI;

@end

@implementation fuse_ext2Pref

- (IBAction) updateButtonClicked: (id) sender
{
}

- (IBAction) removeButtonClicked: (id) sender
{
	NSLog(@"remove button clicked\n");
}

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

- (void) updateGUI
{
	NSString *version;
	NSString *versionString;
	
	version = [self installedVersion];
	versionString = [[NSString alloc] initWithFormat:@"%@ %@", kinstalledString, (version == nil) ? @"unknown" : version];
	
	[aboutLabel setStringValue:kaboutLabelString];
	[installedLabel setStringValue: versionString];
	[updateLabel setStringValue:kupdateString];

	[versionString release];
	[version release];
}

- (void) mainViewDidLoad
{
	[self updateGUI];
}

@synthesize aboutLabel;
@synthesize removeButton;
@synthesize updateButton;
@synthesize installedLabel;
@synthesize updateLabel;

@end
