//
//  fuse_ext2Pref.h
//  fuse-ext2
//
//  Created by Alper Akcan on 4/27/09.
//  Copyright (c) 2009 __MyCompanyName__. All rights reserved.
//

#import <PreferencePanes/PreferencePanes.h>

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
}

- (void) mainViewDidLoad;
- (IBAction) removeButtonClicked: (id) sender;
- (IBAction) updateButtonClicked: (id) sender;

@property (retain) NSTextField *aboutLabel;
@property (retain) NSButton *removeButton;
@property (retain) NSButton *updateButton;
@property (retain) NSTextField *installedLabel;
@property (retain) NSTextField *updateLabel;
@property (retain) NSProgressIndicator *spinnerRemove;
@property (retain) NSProgressIndicator *spinnerUpdate;

@end