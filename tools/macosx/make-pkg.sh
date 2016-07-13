#
# This script is heavly based on ntfs-3g mac osx package,
# thanks to Erik Larsson, and Paul Marks for their efforts.
#

#!/bin/sh

# Exit immediately in case of error
set -e

SUDO="sudo"
SED_E="${SUDO} sed -e"
MV="${SUDO} mv"
CP_R="${SUDO} cp -R"
RM_RF="${SUDO} rm -rf"
MKDIR_P="${SUDO} mkdir -p"
LN_SF="${SUDO} ln -sf"
INSTALL_C="${SUDO} install -c"
CHOWN_R="${SUDO} chown -R"
PKGBUILD="${SUDO} pkgbuild"
TMP_FOLDER="$(mktemp -d)"
DISTRIBUTION_FOLDER="$TMP_FOLDER/Distribution_Folder"
SETFILE="${SUDO} $(xcode-select -p)/Tools/SetFile"

FUSEEXT2_NAME="fuse-ext2"

FUSEEXT2_VERSION="$1"
BUILD_FOLDER="$2"
MKPKG_FOLDER="$3"

if [ x"$FUSEEXT2_VERSION" = x"" ]
then
  echo "Usage: make-pkg.sh <version> <builddir> <mkpkgdir>"
  exit 1
fi

if [ x"$BUILD_FOLDER" = x"" ]
then
  echo "Usage: make-pkg.sh <version> <builddir> <mkpkgdir>"
  exit 1
fi

if [ x"$MKPKG_FOLDER" = x"" ]
then
  echo "Usage: make-pkg.sh <version> <builddir> <mkpkgdir>"
  exit 1
fi

make -f "$BUILD_FOLDER/Makefile" DESTDIR="$DISTRIBUTION_FOLDER" install
${CHOWN_R} root:wheel ${TMP_FOLDER}

${PKGBUILD} --identifier ${FUSEEXT2_NAME} \
            --root "$DISTRIBUTION_FOLDER" \
            --version $FUSEEXT2_VERSION \
            ${FUSEEXT2_NAME}.pkg

${CHOWN_R} root:admin ${FUSEEXT2_NAME}.pkg

sudo hdiutil create -layout NONE -megabytes 1 -fs HFS+ -volname "${FUSEEXT2_NAME}-${FUSEEXT2_VERSION}" "${TMP_FOLDER}/${FUSEEXT2_NAME}-${FUSEEXT2_VERSION}.dmg"
sudo hdiutil attach -private -nobrowse "${TMP_FOLDER}/${FUSEEXT2_NAME}-${FUSEEXT2_VERSION}.dmg"

VOLUME_PATH="/Volumes/${FUSEEXT2_NAME}-${FUSEEXT2_VERSION}"
sudo cp -pRX ${FUSEEXT2_NAME}.pkg "$VOLUME_PATH"

# Set the custom icon.
sudo cp -pRX "${MKPKG_FOLDER}/Install_resources/.VolumeIcon.icns" "${VOLUME_PATH}/.VolumeIcon.icns"
${SETFILE} -a C "$VOLUME_PATH"

# Copy over the license file.
sudo cp "${MKPKG_FOLDER}/Install_resources/README.rtf" "${VOLUME_PATH}/README.rtf"
sudo cp "${MKPKG_FOLDER}/Install_resources/Authors.rtf" "${VOLUME_PATH}/Authors.rtf"
sudo cp "${MKPKG_FOLDER}/Install_resources/ChangeLog.rtf" "${VOLUME_PATH}/ChangeLog.rtf"
sudo cp "${MKPKG_FOLDER}/Install_resources/License.rtf" "${VOLUME_PATH}/License.rtf"

# Detach the volume.
hdiutil detach "$VOLUME_PATH"

# Convert to a read-only compressed dmg.
sudo hdiutil convert -imagekey zlib-level=9 -format UDZO "${TMP_FOLDER}/${FUSEEXT2_NAME}-${FUSEEXT2_VERSION}.dmg" -o "${FUSEEXT2_NAME}-${FUSEEXT2_VERSION}.dmg"

${RM_RF} ${TMP_FOLDER} ${FUSEEXT2_NAME}.pkg

echo "SUCCESS: All Done."
exit 0
