#!/bin/sh
# Run this to generate configure, Makefile.in's, etc

(autoreconf --version) < /dev/null > /dev/null 2>&1 || {
  (autoconf --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have the GNU Build System (autoconf, automake, "
    echo "libtool, etc) to update the build system.  Download the appropriate"
    echo "packages for your distribution, or get the source tar balls from"
    echo "ftp://ftp.gnu.org/pub/gnu/."
    exit 1
  }
  echo
  echo "**Error**: Your version of autoconf is too old (you need at least 2.57)"
  echo "to update the build system.  Download the appropriate updated package"
  echo "for your distribution, or get the source tar ball from"
  echo "ftp://ftp.gnu.org/pub/gnu/."
  exit 1
}

echo Running autoreconf --verbose --install --force
autoreconf --verbose --install --force

echo Removing autom4te.cache
rm -rf autom4te.cache

