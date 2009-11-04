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

static NSString *kreleasePath = @"http://fuse-ext2.svn.sourceforge.net/viewvc/fuse-ext2/release/release.xml";
static NSString *kinstallPath = @"/usr/local/bin/fuse-ext2.install";
static NSString *kuninstallPath = @"/usr/local/bin/fuse-ext2.uninstall";
static NSString *kinstalledPath = @"/Library/Filesystems/fuse-ext2.fs/Contents/Info.plist";
static NSString *kaboutLabelString = @"fuse-ext2 is a ext2/ext3 filesystem support for Fuse. Please visit fuse-ext2 webpage (http://fuse-ext2.sourceforge.net/) for more information.";
static NSString *kinstalledString = @"Installed Version:";
static NSString *kinstallingString = @"Installing new version.";
static NSString *kcheckingString = @"Checking for new version.";
static NSString *kavailableString = @"New version availeble:";
static NSString *kupdateString = @"No Updates Available At This Time.";
static const NSTimeInterval kNetworkTimeOutInterval = 60.00; 

@interface fuse_ext2Pref (PrivateMethods)

- (NSString *) availableVersion;
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
			NSLog(@"fuse-ext2.PrefPane: caught exception %@ when launching task %@", err, task);
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

- (IBAction) updateFuseExt2: (id) sender
{
	int ret;
	NSData *output = nil;
	NSLog(@"fuse-ext2.PrefPane: update button clicked\n");
	if (taskRunning == YES) {
		return;
	}
	[removeButton setEnabled:NO];
	[updateButton setEnabled:NO];
	if (doUpdate == NO) {
		[spinnerUpdate startAnimation:self];
		[updateLabel setStringValue:kcheckingString];
		[self updateGUI];
	} else {
		[spinnerUpdate startAnimation:self];
		[updateLabel setStringValue:kinstallingString];
		ret = [self runTaskForPath:kinstallPath withArguments:[NSArray arrayWithObjects:@"-u", kreleasePath, @"-r", nil] authorized:YES output:&output];
		if (ret != 0) {
			NSLog(@"fuse-ext2.PrefPane: fuse-ext2.install -u -r failed");
		}
		if (output) {
			NSString *string = [[[NSString alloc] initWithData:output encoding:NSUTF8StringEncoding] autorelease];
			NSLog(@"fuse-ext2.PrefPane: output;\n'%@'\n", string);
		}
		[self updateGUI];
	}
}

- (IBAction) removeFuseExt2: (id) sender
{
	int ret;
	int authorized;
	NSData *output = nil;
	if (taskRunning == YES) {
		return;
	}
	[removeButton setEnabled:NO];
	[updateButton setEnabled:NO];
	[spinnerRemove startAnimation:self];
	authorized = [self authorize];
	if (authorized != YES) {
		[spinnerRemove stopAnimation:self];
		[self updateGUI];
		return;
	}
	[updateLabel setStringValue:@"Removing fuse-ext2..."];
	NSLog(@"fuse-ext2.PrefPane: remove button clicked\n");
	ret = [self runTaskForPath:kuninstallPath withArguments:[NSArray arrayWithObjects:@"-u", nil] authorized:YES output:&output];
	NSLog(@"fuse-ext2.PrefPane: runtaskforpath returned:%d\n", ret);
	if (output) {
		NSString *string = [[[NSString alloc] initWithData:output encoding:NSUTF8StringEncoding] autorelease];
		NSLog(@"fuse-ext2.PrefPane: output;\n'%@'\n", string);
	}
	if (ret != 0) {
		[updateLabel setStringValue:@"Removing fuse-ext2 failed, check console log for details."];
	} else {
		[updateLabel setStringValue:@"Removed fuse-ext2."];
	}
	[spinnerRemove stopAnimation:self];
	[self updateGUI];
}

- (NSString *) availableVersion
{
	int ret;
	NSData *output;
	NSString *string;
	NSString *versionString;
	NSString *versionTag;
	NSScanner *dataScanner;
	output = nil;
	versionTag = @"Available Version:";
	ret = [self runTaskForPath:kinstallPath withArguments:[NSArray arrayWithObjects:@"-u", kreleasePath, @"-a", nil] authorized:NO output:&output];
	if (ret != 0) {
		NSLog(@"fuse-ext2.PrefPane: fuse-ext2.install -u -a failed");
		return nil;
	}
	if (output == nil) {
		NSLog(@"fuse-ext2.PrefPane: fuse-ext2.install -u -a failed");
		return nil;
	}
	string = [[NSString alloc] initWithData:output encoding:NSUTF8StringEncoding];
	NSLog(@"fuse-ext2.PrefPane: output;\n'%@'", string);
	dataScanner = [[NSScanner alloc] initWithString:string];
	if ([dataScanner scanString:versionTag intoString:&versionString]) {
		NSLog(@"fuse-ext2.PrefPane: versionString:%@", versionString);
		versionString = nil;
		if ([dataScanner scanUpToString:@"\n" intoString:&versionString]) {
			if (versionString == nil) {
				NSLog(@"fuse-ext2.PrefPane: version is nil");
			} else {
				NSLog(@"fuse-ext2.PrefPane: version is not nil:%@", versionString);
			}
			return [[NSString alloc] initWithString: versionString];
		} else {
			NSLog(@"fuse-ext2.PrefPane: scanuptostring failed");
		}
	} else {
		NSLog(@"fuse-ext2.PrefPane: datascanner failed");
	}
	[string release];
	[dataScanner release];
	return nil;
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
	NSString *available;
	NSString *updateString;
	NSString *versionString;
	
	NSLog(@"fuse-ext2.PrefPane: updating gui\n");
	
	[spinnerUpdate startAnimation:self];

	updateString = nil;
	version = [self installedVersion];
	available = [self availableVersion];
	versionString = [[NSString alloc] initWithFormat:@"%@ %@", kinstalledString, (version == nil) ? @"Not Installed." : version];
	[aboutLabel setStringValue:kaboutLabelString];
	[installedLabel setStringValue: versionString];
	if (available == nil || [version compare:available] == NSOrderedSame) {
		[updateLabel setStringValue:kupdateString];
		[updateButton setTitle:@"Check for Update"];
		doUpdate = NO;
	} else {
		updateString = [[NSString alloc] initWithFormat:@"%@ %@", kavailableString, available];
		[updateLabel setStringValue:updateString];
		[updateButton setTitle:@"Install Update"];
		doUpdate = YES;
	}
	
	[updateString release];
	[versionString release];
	[version release];
	[available release];
	
	[removeButton setEnabled:YES];
	[updateButton setEnabled:YES];
	
	[spinnerUpdate stopAnimation:self];

	NSLog(@"fuse-ext2.PrefPane: gui updated\n");
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
