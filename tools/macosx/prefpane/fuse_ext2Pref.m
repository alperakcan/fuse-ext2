//
//  fuse_ext2Pref.m
//  fuse-ext2
//
//  Created by Alper Akcan on 4/27/09.
//
// Authorization related functions
//  authorize
//  copyRights
//  deauthorize
// are heavly based on MacFUSE PrefPane implementation
//

#import "fuse_ext2Pref.h"

#import <Carbon/Carbon.h>

static NSString *kinstalledPath = @"/Library/Filesystems/fuse-ext2.fs/Contents/Info.plist";
static NSString *kaboutLabelString = @"fuse-ext2 is a ext2/ext3 filesystem support for Fuse. Please visit fuse-ext2 homepage for more information.";
static NSString *kinstalledString = @"Installed Version:";
static NSString *kupdateString = @"No Updates Available At This Time.";

@interface fuse_ext2Pref (PrivateMethods)

- (NSString *) installedVersion;
- (void) updateGUI;

- (BOOL) authorize;
- (BOOL) copyRights;
- (void) deauthorize;

@end

@implementation fuse_ext2Pref

- (IBAction) updateButtonClicked: (id) sender
{
	BOOL authorized;
	[spinnerUpdate startAnimation:self];
	NSLog(@"update button clicked\n");
	authorized = [self authorize];
	if (authorized != YES) {
		[spinnerUpdate stopAnimation:self];
	}
	[spinnerUpdate stopAnimation:self];
}

- (IBAction) removeButtonClicked: (id) sender
{
	BOOL authorized;
	[spinnerRemove startAnimation:self];
	NSLog(@"remove button clicked\n");
	authorized = [self authorize];
	if (authorized != YES) {
		[spinnerRemove stopAnimation:self];
	}
	[spinnerRemove stopAnimation:self];
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

- (BOOL) authorize
{
	BOOL authorized;
	OSStatus status;
	AuthorizationRights *kNoRightsSpecified;

	authorized = NO;
	
	if (authorizationReference != nil) {
		authorized = [self copyRights];
	} else {
		kNoRightsSpecified = nil;
		status = AuthorizationCreate(kNoRightsSpecified, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &authorizationReference);
		if (status == errAuthorizationSuccess) {
			authorized = [self copyRights];
		}
	}
	return authorized;
}

- (BOOL) copyRights
{
	NSParameterAssert(authorizationReference);

	BOOL isGood;
	OSStatus status;
	
	AuthorizationFlags authorizationFlags =
		kAuthorizationFlagDefaults |
		kAuthorizationFlagPreAuthorize |
		kAuthorizationFlagExtendRights |
		kAuthorizationFlagInteractionAllowed;
	AuthorizationItem authorizationItem = {
		kAuthorizationRightExecute,
		0,
		NULL,
		0,
	};
	AuthorizationRights authorizationRights = {
		1,
		&authorizationItem,
	};
	
	isGood = NO;
	status = AuthorizationCopyRights(
		authorizationReference,
		&authorizationRights,
		kAuthorizationEmptyEnvironment,
		authorizationFlags,
		NULL);
	if (status != errAuthorizationSuccess) {
		[self deauthorize];
	} else {
		isGood = YES;
	}
	
	return isGood;
}

- (void) deauthorize
{
	if (authorizationReference != nil) {
		AuthorizationFree(authorizationReference, kAuthorizationFlagDefaults);
		authorizationReference = nil;
	}
}

- (void) mainViewDidLoad
{
	[self updateGUI];
}

- (void) dealloc
{
	[self deauthorize];
	[super dealloc];
}

@end
