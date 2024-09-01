#!/bin/bash
if [ `id -u` != 0 ]; then
	echo "You are not running this script as root. Try it again as root or with \"sudo ./make_flash.sh\"."
	echo "Bye Bye..."
	exit
fi

BOXTYPE=$2
CURDIR=`pwd`
BASEDIR=$CURDIR/../..

TUFSBOXDIR=$BASEDIR/tufsbox/$BOXTYPE
SCRIPTDIR=$CURDIR/scripts
TMPDIR=$CURDIR/tmp
TMPROOTDIR=$TMPDIR/ROOT
TMPKERNELDIR=$TMPDIR/KERNEL
OUTDIR=$TUFSBOXDIR/image
TMPSTORAGEDIR=$TMPDIR/STORAGE

if [ -e $TMPDIR ]; then
	rm -rf $TMPDIR/*
fi

mkdir -p $TMPDIR
mkdir -p $TMPROOTDIR
mkdir -p $TMPKERNELDIR
mkdir -p $TMPSTORAGEDIR

echo ""
echo "-----------------------------------------------------------------------"
echo "It's expected that an image was already build prior to this execution!"
echo "-----------------------------------------------------------------------"
echo "Checking target..."
$SCRIPTDIR/prepare_root.sh $CURDIR $TUFSBOXDIR/release $TMPROOTDIR $TMPKERNELDIR $TMPSTORAGEDIR
echo "Root prepared"
echo ""
echo "You can customize your image now (i.e. move files you like from ROOT to STORAGE)."
echo "Or insert your changes into scripts/customize.sh"
$SCRIPTDIR/customize.sh $CURDIR $TMPROOTDIR $TMPKERNELDIR $TMPSTORAGEDIR
echo "-----------------------------------------------------------------------"
echo "Creating flash image..."
$SCRIPTDIR/flash_part_w_fw.sh $CURDIR $TUFSBOXDIR $OUTDIR $TMPROOTDIR $TMPKERNELDIR $TMPSTORAGEDIR
echo "-----------------------------------------------------------------------"

AUDIOELFSIZE=`stat -c %s $TMPROOTDIR/lib/firmware/audio.elf`
if [ "$AUDIOELFSIZE" == "0" -o "$AUDIOELFSIZE" == "" ]; then
	echo -e "\033[01;31m"
	echo "!!! WARNING: AUDIOELF SIZE IS ZERO OR MISSING !!!"
	echo "IF YOUR ARE CREATING THE FW PART MAKE SURE THAT YOU USE CORRECT ELFS"
	echo -e "\033[00m"
fi

VIDEOELFSIZE=`stat -c %s $TMPROOTDIR/lib/firmware/video.elf`
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
echo "UFS910:      To flash the created image rename the *.img file to miniFLASH.img"
echo "             and copy it to the root (/) of your usb drive."
echo "             To start the flashing process press RECORD for 10 sec on your"
echo "             remote control while the box is starting."
echo ""
echo "MINI/MINI2:  To flash the created image rename the *.img file to usb_update.img"
echo "             and copy it to the root (/) of your usb drive."
echo "             To start the flashing process press POWER for 10 sec on your"
echo "             box while the box is starting."
echo ""
