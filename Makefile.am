
SUBDIRS  =
SUBDIRS += fuse-ext2
SUBDIRS += tools
SUBDIRS += test
SUBDIRS += test/pjdfstest

EXTRA_DIST = \
	fuse-ext2.pc.in

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = fuse-ext2.pc

$(pkgconfig_DATA): config.status

man1_MANS = fuse-ext2.1

if DARWIN
package:
	$(SHELL) $(top_srcdir)/tools/macosx/make-pkg.sh "$(VERSION)" "$(top_srcdir)" "$(top_builddir)" "$(top_srcdir)/tools/macosx/"
endif
if LINUX
package:
	echo "You can use checkinstall or some other equivalent tool to generate install package for your distribution."
endif
