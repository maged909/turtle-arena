#!/bin/sh
#

# Let's make the user give us a target build system

if [ $# -ne 1 ]; then
	echo "Usage:   $0 target_architecture"
	echo "Example: $0 i386"
	echo "other valid options are x86_64 or ppc"
	echo
	echo "If you don't know or care about architectures please consider using make-macosx-ub.sh instead of this script."
	exit 1
fi

if [ "$1" == "i386" ]; then
	BUILDARCH=i386
elif [ "$1" == "x86_64" ]; then
	BUILDARCH=x86_64
elif [ "$1" == "ppc" ]; then
	BUILDARCH=ppc
else
	echo "Invalid architecture: $1"
	echo "Valid architectures are i386, x86_64 or ppc"
	exit 1
fi

CC=gcc-4.0
APPBUNDLE=turtlearena.app
BINARY=turtlearena.${BUILDARCH}
DEDBIN=turtlearena-server.${BUILDARCH}
PKGINFO=APPLTUAR
ICNS=misc/quake3.icns
DESTDIR=build/release-darwin-${BUILDARCH}
BASEDIR=baseq3
MPACKDIR=missionpack

BIN_OBJ="
	build/release-darwin-${BUILDARCH}/turtlearena.${BUILDARCH}
"
BIN_DEDOBJ="
	build/release-darwin-${BUILDARCH}/turtlearena-server.${BUILDARCH}
"
BASE_OBJ="
	build/release-darwin-${BUILDARCH}/$BASEDIR/cgame${BUILDARCH}.dylib
	build/release-darwin-${BUILDARCH}/$BASEDIR/ui${BUILDARCH}.dylib
	build/release-darwin-${BUILDARCH}/$BASEDIR/game${BUILDARCH}.dylib
	../install/$BASEDIR/assets0.pk3
	../install/$BASEDIR/assets1-qvms.pk3
	../install/$BASEDIR/assets2-music.pk3
"

cd `dirname $0`
if [ ! -f Makefile ]; then
	echo "This script must be run from the Turtle Arena build directory"
	exit 1
fi

# Build game assets if needed.
if [ ! -f ../install/$BASEDIR/assets0.pk3 ]; then
        echo "Building assets..."
        (make -C .. assets) || exit 1;
        if [ ! -f ../install/$BASEDIR/assets0.pk3 ]; then
                echo "Error: Failed to build assets"
                exit 1
        fi
fi

Q3_VERSION=`grep '^VERSION=' Makefile | sed -e 's/.*=\(.*\)/\1/'`

# We only care if we're >= 10.4, not if we're specifically Tiger.
# "8" is the Darwin major kernel version.
TIGERHOST=`uname -r |perl -w -p -e 's/\A(\d+)\..*\Z/$1/; $_ = (($_ >= 8) ? "1" : "0");'`

# we want to use the oldest available SDK for max compatiblity. However 10.4 and older
# can not build 64bit binaries, making 10.5 the minimum version.   This has been tested 
# with xcode 3.1 (xcode31_2199_developerdvd.dmg).  It contains the 10.5 SDK and a decent
# enough gcc to actually compile ioquake3
# For PPC macs, G4's or better are required to run ioquake3.

unset ARCH_SDK
unset ARCH_CFLAGS
unset ARCH_LDFLAGS

if [ -d /Developer/SDKs/MacOSX10.5.sdk ]; then
	ARCH_SDK=/Developer/SDKs/MacOSX10.5.sdk
	ARCH_CFLAGS="-arch ${BUILDARCH} -isysroot /Developer/SDKs/MacOSX10.5.sdk \
			-DMAC_OS_X_VERSION_MIN_REQUIRED=1050"
	ARCH_LDFLAGS=" -mmacosx-version-min=10.5"
fi


echo "Building ${BUILDARCH} Client/Dedicated Server against \"$ARCH_SDK\""
sleep 3

if [ ! -d $DESTDIR ]; then
	mkdir -p $DESTDIR
fi

# For parallel make on multicore boxes...
NCPU=`sysctl -n hw.ncpu`


# intel client and server
if [ -d build/release-darwin-x86_64 ]; then
	rm -r build/release-darwin-x86_64
fi
(ARCH=x86_64 CFLAGS=$X86_CFLAGS LDFLAGS=$X86_LDFLAGS make -j$NCPU) || exit 1;

echo "Creating .app bundle $DESTDIR/$APPBUNDLE"
if [ ! -d $DESTDIR/$APPBUNDLE/Contents/MacOS/$BASEDIR ]; then
	mkdir -p $DESTDIR/$APPBUNDLE/Contents/MacOS/$BASEDIR || exit 1;
fi
if [ ! -d $DESTDIR/$APPBUNDLE/Contents/Resources ]; then
	mkdir -p $DESTDIR/$APPBUNDLE/Contents/Resources
fi
cp $ICNS $DESTDIR/$APPBUNDLE/Contents/Resources/turtlearena.icns || exit 1;
echo $PKGINFO > $DESTDIR/$APPBUNDLE/Contents/PkgInfo
echo "
	<?xml version=\"1.0\" encoding=\"UTF-8\"?>
	<!DOCTYPE plist
		PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\"
		\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
	<plist version=\"1.0\">
	<dict>
		<key>CFBundleDevelopmentRegion</key>
		<string>English</string>
		<key>CFBundleExecutable</key>
		<string>$BINARY</string>
		<key>CFBundleGetInfoString</key>
		<string>Turtle Arena $Q3_VERSION</string>
		<key>CFBundleIconFile</key>
		<string>turtlearena.icns</string>
		<key>CFBundleIdentifier</key>
		<string>org.turtlearena.turtlearena</string>
		<key>CFBundleInfoDictionaryVersion</key>
		<string>6.0</string>
		<key>CFBundleName</key>
		<string>Turtle Arena</string>
		<key>CFBundlePackageType</key>
		<string>APPL</string>
		<key>CFBundleShortVersionString</key>
		<string>$Q3_VERSION</string>
		<key>CFBundleSignature</key>
		<string>$PKGINFO</string>
		<key>CFBundleVersion</key>
		<string>$Q3_VERSION</string>
		<key>NSExtensions</key>
		<dict/>
		<key>NSPrincipalClass</key>
		<string>NSApplication</string>
	</dict>
	</plist>
	" > $DESTDIR/$APPBUNDLE/Contents/Info.plist

lipo -create -o $DESTDIR/$APPBUNDLE/Contents/MacOS/$BINARY $BIN_OBJ
lipo -create -o $DESTDIR/$APPBUNDLE/Contents/MacOS/$DEDBIN $BIN_DEDOBJ
cp $BASE_OBJ $DESTDIR/$APPBUNDLE/Contents/MacOS/$BASEDIR/
cp code/libs/macosx/*.dylib $DESTDIR/$APPBUNDLE/Contents/MacOS/

