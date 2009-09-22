//
//  fuse_ext2Pref.h
//  fuse-ext2
//
//  Created by Alper Akcan on 4/27/09.
//

#import <PreferencePanes/PreferencePanes.h>
#import <Security/Security.h>

@interface fuse_ext2Pref : NSPreferencePane 
{
@private
	IBOutlet NSTextField *aboutLabel;
	IBOutlet NSButton *removeButton;
	IBOutlet NSButton *updateButton;
	IBOutlet NSTextField *installedLabel;
	IBOutlet NSTextField *updateLabel;
	IBOutlet NSProgressIndicator *spinnerRemove;
	IBOutlet NSProgressIndicator *spinnerUpdate;
	AuthorizationRef authorizationReference;
	BOOL taskRunning;
	BOOL doUpdate;
}

- (void) mainViewDidLoad;
- (IBAction) updateFuseExt2: (id) sender;
- (IBAction) removeFuseExt2: (id) sender;

@end
