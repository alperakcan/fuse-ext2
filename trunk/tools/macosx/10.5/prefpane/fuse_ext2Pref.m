//
//  fuse_ext2Pref.m
//  fuse-ext2
//
//  Created by Alper Akcan on 4/27/09.
//  Copyright (c) 2009 __MyCompanyName__. All rights reserved.
//

#import "fuse_ext2Pref.h"

static NSString *kaboutLabelString = @"fuse-ext2 is a ext2/ext3 filesystem support for Fuse. Please visit fuse-ext2 homepage for more information.";
static NSString *kinstalledString = @"Installed Version:";
static NSString *kupdateString = @"No Updates Available At This Time";

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
}

- (NSString *) installedVersion
{
	NSString *versionString;
	versionString = [[NSString alloc] initWithFormat:@"%@ %@", kinstalledString, @"0.0.3"];
	return versionString;
}

- (void) updateGUI
{
	NSString *version;
	version = [self installedVersion];
	[aboutLabel setStringValue:kaboutLabelString];
	[installedLabel setStringValue:version];
	[updateLabel setStringValue:kupdateString];
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
