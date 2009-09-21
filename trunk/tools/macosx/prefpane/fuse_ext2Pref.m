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
//  runTaskForPath
// are heavly based on MacFUSE PrefPane implementation
//

#import "fuse_ext2Pref.h"

#import <Carbon/Carbon.h>

static NSString *kuninstallPath = @"/usr/local/bin/fuse-ext2.uninstall";
static NSString *kinstalledPath = @"/Library/Filesystems/fuse-ext2.fs/Contents/Info.plist";
static NSString *kaboutLabelString = @"fuse-ext2 is a ext2/ext3 filesystem support for Fuse. Please visit fuse-ext2 homepage for more information.";
static NSString *kinstalledString = @"Installed Version:";
static NSString *kupdateString = @"No Updates Available At This Time.";
static const NSTimeInterval kNetworkTimeOutInterval = 15; 

@interface fuse_ext2Pref (PrivateMethods)

- (NSString *) installedVersion;
- (void) updateGUI;

- (BOOL) authorize;
- (BOOL) copyRights;
- (void) deauthorize;

- (BOOL) isTaskRunning;
- (void) setTaskRunning: (BOOL) value;
- (int) runTaskForPath:(NSString *) path withArguments:(NSArray *) arguments authorized:(BOOL) authorized output:(NSData **) output;

@end

@implementation fuse_ext2Pref

- (BOOL) isTaskRunning
{
	return taskRunning;
}

- (void) setTaskRunning: (BOOL) value
{
	if (taskRunning != value) {
		taskRunning = value;
	}
}

- (int) runTaskForPath:(NSString *) path withArguments:(NSArray *) arguments authorized:(BOOL) authorized output:(NSData **) output
{
	int result = 0;
	NSFileHandle *outFile = nil;
	[self setTaskRunning:YES];
	if (authorized == NO) {
		// non-authorized 
		NSTask* task = [[[NSTask alloc] init] autorelease];
		[task setLaunchPath:path];
		[task setArguments:arguments];
		[task setEnvironment:[NSDictionary dictionary]];
		NSPipe *outPipe = [NSPipe pipe];    
		[task setStandardOutput:outPipe];
		
		@try {
			[task launch];
		} @catch (NSException *err) {
			NSLog(@"caught exception %@ when launching task %@", err, task);
			[self setTaskRunning:NO];
			return -1;
		} 
		
		NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
		NSDate *startDate = [NSDate date];
		do {
			NSDate *waitDate = [NSDate dateWithTimeIntervalSinceNow:0.01];
			if ([waitDate timeIntervalSinceDate:startDate] > kNetworkTimeOutInterval) {
				result = -1;
				[task terminate];
			}
			[runLoop runUntilDate:waitDate];
		} while ([task isRunning]);
		
		if (result == 0) {
			result = [task terminationStatus];
		}
		if (output) {
			outFile = [outPipe fileHandleForReading];
		}
	} else {
		// authorized
		if ([self authorize] == NO) {
			return -1;
		}
		FILE *outPipe = NULL;
		unsigned int numArgs = [arguments count];
		const char **args = malloc(sizeof(char*) * (numArgs + 1));
		if (!args) {
			[self setTaskRunning:NO];
			return -1;
		}
		const char *cPath = [path fileSystemRepresentation];
		for (unsigned int i = 0; i < numArgs; i++) {
			args[i] = [[arguments objectAtIndex:i] fileSystemRepresentation];
		}
		
		args[numArgs] = NULL;
		
		AuthorizationFlags myFlags = kAuthorizationFlagDefaults; 
		result = AuthorizationExecuteWithPrivileges(authorizationReference,  cPath, myFlags, (char * const *) args, &outPipe);
		free(args);
		if (result == 0) {
			int wait_status;
			int pid = 0;
			NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
			do {
				NSDate *waitDate = [NSDate dateWithTimeIntervalSinceNow:0.1];
				[runLoop runUntilDate:waitDate];
				pid = waitpid(-1, &wait_status, WNOHANG);
			} while (pid == 0);
			if (pid == -1 || !WIFEXITED(wait_status)) {
				result = -1;
			} else {
				result = WEXITSTATUS(wait_status);
			}
			if (output) {
				int fd = fileno(outPipe);
				outFile = [[[NSFileHandle alloc] initWithFileDescriptor:fd closeOnDealloc:YES] autorelease];
			}
		}
	}
	if (outFile && output) {
		*output = [outFile readDataToEndOfFile];
	}      
	[self setTaskRunning:NO];
	return result;
}

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
	int ret;
	int authorized;
	NSData *output = nil;
	[spinnerRemove startAnimation:self];
	authorized = [self authorize];
	if (authorized != YES) {
		[spinnerRemove stopAnimation:self];
		return;
	}
	[updateLabel setStringValue:@"Removing fuse-ext2..."];
	NSLog(@"remove button clicked\n");
	ret = [self runTaskForPath:kuninstallPath withArguments:[NSArray arrayWithObjects:nil] authorized:YES output:&output];
	NSLog(@"runtaskforpath returned:%d\n", ret);
	if (output) {
		NSString *string = [[[NSString alloc] initWithData:output encoding:NSUTF8StringEncoding] autorelease];
		NSLog(@"output: %@\n", string);
	}
	if (ret != 0) {
		[updateLabel setStringValue:@"Removing fuse-ext2 failed, check console log for details."];
	} else {
		[updateLabel setStringValue:@"Removed fuse-ext2."];
	}
	[spinnerRemove stopAnimation:self];
	[self updateGUI];
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
		bundleVersion = [fuse_ext2Plist objectForKey:(NSString *) kCFBundleVersionKey];
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
