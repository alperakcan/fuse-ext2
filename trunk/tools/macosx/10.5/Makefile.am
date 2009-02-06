
install-exec-local:
if DARWIN
	$(INSTALL) -d /System/Library/Filesystems/fuse-ext2.fs
	$(INSTALL) -d /System/Library/Filesystems/fuse-ext2.fs/Contents
	$(INSTALL) -d /System/Library/Filesystems/fuse-ext2.fs/Contents/Resources
	$(INSTALL) -d /System/Library/Filesystems/fuse-ext2.fs/Contents/Resources/English.lproj
	$(INSTALL) -m 755 $(top_srcdir)/tools/macosx/10.5/fuse-ext2.fs/fuse-ext2.util /System/Library/Filesystems/fuse-ext2.fs/fuse-ext2.util
	$(INSTALL) -m 755 $(top_srcdir)/tools/macosx/10.5/fuse-ext2.fs/mount_fuse-ext2 /System/Library/Filesystems/fuse-ext2.fs/mount_fuse-ext2
	$(INSTALL) -m 644 $(top_srcdir)/tools/macosx/10.5/fuse-ext2.fs/Contents/Info.plist /System/Library/Filesystems/fuse-ext2.fs/Contents/Info.plist
	$(INSTALL) -m 644 $(top_srcdir)/tools/macosx/10.5/fuse-ext2.fs/Contents/PkgInfo /System/Library/Filesystems/fuse-ext2.fs/Contents/PkgInfo
	$(INSTALL) -m 644 $(top_srcdir)/tools/macosx/10.5/fuse-ext2.fs/Contents/Resources/English.lproj/InfoPlist.strings /System/Library/Filesystems/fuse-ext2.fs/Contents/Resources/English.lproj/InfoPlist.strings
else
	@true
endif