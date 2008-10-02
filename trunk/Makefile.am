
SUBDIRS  = e2fsprogs-1.41.1
SUBDIRS += fuse-ext2
SUBDIRS += test/pjd-fstest-20080816

EXTRA_DIST =		\
		fuse-ext2.pc.in

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = fuse-ext2.pc

$(pkgconfig_DATA): config.status

if DARWIN
package:
		$(SHELL) $(top_srcdir)/tools/macosx/10.5/make-pkg.sh "$(VERSION)" "$(top_builddir)" "$(top_srcdir)/tools/macosx/10.5/"
endif
if LINUX
package:
		echo "You can use checkinstall or some other equivalent tool to generate install package for your distribution."
endif
