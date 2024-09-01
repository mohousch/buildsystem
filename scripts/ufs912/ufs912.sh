#!/bin/bash
if [ `id -u` != 0 ]; then
	echo "You are not running this script as root. Try it again as root or with \"sudo ./ufs912.sh\"."
	echo "Bye Bye..."
	exit
fi

CURDIR=`pwd`
BASEDIR=$CURDIR/../..

TUFSBOXDIR=$BASEDIR/tufsbox/ufs912
SCRIPTDIR=$CURDIR/scripts
TMPDIR=$CURDIR/tmp
TMPROOTDIR=$TMPDIR/ROOT
TMPKERNELDIR=$TMPDIR/KERNEL
OUTDIR=$TUFSBOXDIR/image
TMPFWDIR=$TMPDIR/FW

if [ -e $TMPDIR ]; then
	rm -rf $TMPDIR/*
fi

mkdir -p $TMPDIR
mkdir -p $TMPROOTDIR
mkdir -p $TMPKERNELDIR
mkdir -p $TMPFWDIR

echo ""
echo "-----------------------------------------------------------------------"
echo "It's expected that an image was already build prior to this execution!"
echo "-----------------------------------------------------------------------"
echo "Checking target..."
$SCRIPTDIR/prepare_root.sh $CURDIR $TUFSBOXDIR/release $TMPROOTDIR $TMPKERNELDIR $TMPFWDIR
echo "Root prepared"
echo "-----------------------------------------------------------------------"
echo "Creating flash image..."
$SCRIPTDIR/flash_part_w_fw.sh $CURDIR $TUFSBOXDIR $OUTDIR $TMPROOTDIR $TMPKERNELDIR $TMPFWDIR
echo "-----------------------------------------------------------------------"

AUDIOELFSIZE=`stat -c %s $TMPFWDIR/audio.elf`
if [ "$AUDIOELFSIZE" == "0" -o "$AUDIOELFSIZE" == "" ]; then
	echo -e "\033[01;31m"
	echo "!!! WARNING: AUDIOELF SIZE IS ZERO OR MISSING !!!"
	echo "IF YOUR ARE CREATING THE FW PART MAKE SURE THAT YOU USE CORRECT ELFS"
	echo -e "\033[00m"
fi

VIDEOELFSIZE=`stat -c %s $TMPFWDIR/video.elf`
if [ "$VIDEOELFSIZE" == "0" -o "$VIDEOELFSIZE" == "" ]; then
	echo -e "\033[01;31m"
	echo "!!! WARNING: VIDEOELF SIZE IS ZERO OR MISSING !!!"
	echo "IF YOUR ARE CREATING THE FW PART MAKE SURE THAT YOU USE CORRECT ELFS"
	echo -e "\033[00m"
fi

if [ ! -e $TMPROOTDIR/dev/mtd0 ]; then
	echo -e "\033[01;31m"
	echo "!!! WARNING: DEVS ARE MISSING !!!"
	echo "IF YOUR ARE CREATING THE ROOT PART MAKE SURE THAT YOU USE A CORRECT DEV.TAR"
	echo -e "\033[00m"
fi

[ "$1" != "" ] && chown -R $1:users $OUTDIR/

echo "Flashimage created:"
echo ""
echo "To flash the created image copy the *.img file to"
echo "your usb drive in the subfolder /kathrein/ufs912/"
echo ""
echo "Remember, if this is the first time you flash Enigma2 or Neutrino,"
echo "you will need to flash the updatescript.sh first."
echo "The script can be found in the extras folder, copy it to /kathrein/ufs912/ on your usb drive."
echo ""
