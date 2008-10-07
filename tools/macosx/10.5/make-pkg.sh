#!/bin/sh

SUDO="sudo"
SED_E="${SUDO} sed -e"
MV="${SUDO} mv"
RM_RF="${SUDO} rm -rf"
MKDIR_P="${SUDO} mkdir -p"
LN_SF="${SUDO} ln -sf"
INSTALL_C="${SUDO} install -c"
CHOWN_R="${SUDO} chown -R"
PKGMANAGER="${SUDO} /Developer/usr/bin/packagemaker"
TMP_FOLDER="tmp/"
DISTRIBUTION_FOLDER="${TMP_FOLDER}/Distribution_folder/"

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

${RM_RF} ${TMP_FOLDER} *.pkg *.dmg
${MKDIR_P} ${TMP_FOLDER}
${MKDIR_P} ${DISTRIBUTION_FOLDER}
${MKDIR_P} ${DISTRIBUTION_FOLDER}/usr
${MKDIR_P} ${DISTRIBUTION_FOLDER}/usr/sbin
${MKDIR_P} ${DISTRIBUTION_FOLDER}/usr/local
${MKDIR_P} ${DISTRIBUTION_FOLDER}/usr/local/bin
${MKDIR_P} ${DISTRIBUTION_FOLDER}/usr/local/lib
${MKDIR_P} ${DISTRIBUTION_FOLDER}/usr/local/lib/pkgconfig
${INSTALL_C} -m 731 ${BUILD_FOLDER}/fuse-ext2/fuse-ext2 ${DISTRIBUTION_FOLDER}/usr/local/bin/fuse-ext2
${INSTALL_C} -m 731 ${BUILD_FOLDER}/fuse-ext2/fuse-ext2.probe ${DISTRIBUTION_FOLDER}/usr/local/bin/fuse-ext2.probe
${INSTALL_C} -m 644 ${BUILD_FOLDER}/fuse-ext2.pc ${DISTRIBUTION_FOLDER}/usr/local/lib/pkgconfig/fuse-ext2.pc
${SED_E} "s/FUSEEXT2_VERSION_LITERAL/0/g" < ${MKPKG_FOLDER}/Info.plist.in > ${MKPKG_FOLDER}/Info.plist
${MV} ${MKPKG_FOLDER}/Info.plist ${TMP_FOLDER}/Info.plist
${CHOWN_R} root:wheel ${TMP_FOLDER}
${SUDO} find ${DISTRIBUTION_FOLDER} -name ".svn" -type d | xargs ${SUDO} rm
${PKGMANAGER} -build -p ${FUSEEXT2_NAME}.pkg \
			  -f ${DISTRIBUTION_FOLDER} \
			  -b ./ \
			  -ds \
			  -v \
			  -r ${MKPKG_FOLDER}/Install_resources/ \
			  -i ${TMP_FOLDER}/Info.plist \
			  -d ${MKPKG_FOLDER}/Description.plist
${CHOWN_R} root:admin ${FUSEEXT2_NAME}.pkg

sudo hdiutil create -layout NONE -megabytes 1 -fs HFS+ -volname "${FUSEEXT2_NAME}" "${TMP_FOLDER}/${FUSEEXT2_NAME}.dmg"
sudo hdiutil attach -private -nobrowse "${TMP_FOLDER}/${FUSEEXT2_NAME}.dmg"

VOLUME_PATH="/Volumes/${FUSEEXT2_NAME}"
sudo cp -pRX ${FUSEEXT2_NAME}.pkg "$VOLUME_PATH"

# Set the custom icon.
sudo cp -pRX "${MKPKG_FOLDER}/Install_resources/.VolumeIcon.icns" "$VOLUME_PATH"/.VolumeIcon.icns
sudo /Developer/Tools/SetFile -a C "$VOLUME_PATH"

# Copy over the license file.
sudo cp "${MKPKG_FOLDER}/Install_resources/License.rtf" "$VOLUME_PATH"/License.rtf

# Detach the volume.
hdiutil detach "$VOLUME_PATH"

# Convert to a read-only compressed dmg.
hdiutil convert -imagekey zlib-level=9 -format UDZO "${TMP_FOLDER}/${FUSEEXT2_NAME}.dmg" -o "${FUSEEXT2_NAME}.dmg"

${RM_RF} ${TMP_FOLDER} ${FUSEEXT2_NAME}.pkg 

echo "SUCCESS: All Done."
exit 0
