
if DARWIN

noinst_PROGRAMS = \
	fuse-ext2.install \
	fuse-ext2.uninstall

fuse_ext2_install_SOURCES = \
	fuse-ext2.install.c

fuse_ext2_install_CFLAGS = \
	-ObjC

fuse_ext2_install_LDFLAGS = \
	-framework Foundation

fuse_ext2_uninstall_SOURCES = \
	fuse-ext2.uninstall.c

install-exec-local:
	$(INSTALL) -d $(DESTDIR)/System/Library/Filesystems/fuse-ext2.fs
	$(INSTALL) -d $(DESTDIR)/System/Library/Filesystems/fuse-ext2.fs/Support
	$(INSTALL) -d $(DESTDIR)/System/Library/Filesystems/fuse-ext2.fs/Contents
	$(INSTALL) -d $(DESTDIR)/System/Library/Filesystems/fuse-ext2.fs/Contents/Resources
	$(INSTALL) -d $(DESTDIR)/System/Library/Filesystems/fuse-ext2.fs/Contents/Resources/English.lproj
	$(INSTALL) -m 755 $(top_builddir)/tools/macosx/fuse-ext2.install $(DESTDIR)/System/Library/Filesystems/fuse-ext2.fs/Support/fuse-ext2.install
	$(INSTALL) -m 755 $(top_builddir)/tools/macosx/fuse-ext2.uninstall $(DESTDIR)/System/Library/Filesystems/fuse-ext2.fs/Support/fuse-ext2.uninstall
	$(INSTALL) -m 755 $(top_srcdir)/tools/macosx/fuse-ext2.fs/fuse-ext2.util $(DESTDIR)/System/Library/Filesystems/fuse-ext2.fs/fuse-ext2.util
	$(INSTALL) -m 755 $(top_srcdir)/tools/macosx/fuse-ext2.fs/mount_fuse-ext2 $(DESTDIR)/System/Library/Filesystems/fuse-ext2.fs/mount_fuse-ext2
	sed "s/FUSEEXT2_VERSION_LITERAL/$(VERSION)/g" < $(top_srcdir)/tools/macosx/fuse-ext2.fs/Contents/Info.plist.in > $(top_srcdir)/tools/macosx/fuse-ext2.fs/Contents/Info.plist
	$(INSTALL) -m 644 $(top_srcdir)/tools/macosx/fuse-ext2.fs/Contents/Info.plist $(DESTDIR)/System/Library/Filesystems/fuse-ext2.fs/Contents/Info.plist
	$(INSTALL) -m 644 $(top_srcdir)/tools/macosx/fuse-ext2.fs/Contents/PkgInfo $(DESTDIR)/System/Library/Filesystems/fuse-ext2.fs/Contents/PkgInfo
	$(INSTALL) -m 644 $(top_srcdir)/tools/macosx/fuse-ext2.fs/Contents/Resources/English.lproj/InfoPlist.strings $(DESTDIR)/System/Library/Filesystems/fuse-ext2.fs/Contents/Resources/English.lproj/InfoPlist.strings

endif
