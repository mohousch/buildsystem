#!/bin/bash

CURDIR=$1
TUFSBOXDIR=$2
OUTDIR=$3
TMPROOTDIR=$4
TMPKERNELDIR=$5

DATETIME=_`date +%d.%m.%Y-%H.%M`

echo "CURDIR       = $CURDIR"
echo "TUFSBOXDIR   = $TUFSBOXDIR"
echo "OUTDIR       = $OUTDIR"
echo "TMPROOTDIR   = $TMPROOTDIR"
echo "TMPKERNELDIR = $TMPKERNELDIR"

MKSQUASHFS=$TUFSBOXDIR/host/bin/mksquashfs
MKFSJFFS2=$TUFSBOXDIR/host/bin/mkfs.jffs2
SUMTOOL=$TUFSBOXDIR/host/bin/sumtool
PAD=$TUFSBOXDIR/host/bin/pad

if [ -f $TMPROOTDIR/etc/hostname ]; then
	BOXTYPE=`cat $TMPROOTDIR/etc/hostname`
elif [ -f $TMPROOTDIR/var/etc/hostname ]; then
	BOXTYPE=`cat $TMPROOTDIR/var/etc/hostname`
fi

#
OUTFILE=$OUTDIR/$BOXTYPE$DATETIME

if [ ! -e $OUTDIR ]; then
	mkdir $OUTDIR
fi

if [ -e $OUTFILE ]; then
	rm -f $OUTFILE
	rm -f $OUTFILE.md5
fi

# --- KERNEL ---
# Size 8MB !
cp -f $TMPKERNELDIR/uImage $OUTDIR/uImage

# --- ROOT ---
# Size 64MB !
echo "MKFSJFFS2 -r $TMPROOTDIR -o $CURDIR/mtd_root.bin -e 0x20000 -p -n"
$MKFSJFFS2 -r $TMPROOTDIR -o $CURDIR/mtd_root.bin -e 0x20000 -p -n
echo "SUMTOOL -p -e 0x20000 -i $CURDIR/mtd_root.bin -o $CURDIR/mtd_root.sum.bin"
$SUMTOOL -p -e 0x20000 -i $CURDIR/mtd_root.bin -o $CURDIR/mtd_root.sum.bin

rm -f $CURDIR/mtd_root.bin

SIZE=`stat mtd_root.sum.bin -t --format %s`
SIZE=`printf "0x%07x" $SIZE`
if [[ $SIZE > "0x4000000" ]]; then
	echo "ROOT TO BIG. $SIZE instead of 0x4000000" > /dev/stderr
	read -p "Press ENTER to continue..."
fi

mv $CURDIR/mtd_root.sum.bin $OUTDIR/e2jffs2.img

rm -f $CURDIR/mtd_kernel.pad.bin
rm -f $CURDIR/mtd_root.sum.bin

cd $OUTDIR
zip -j $OUTFILE.zip e2jffs2.img uImage
rm -f $OUTDIR/e2jffs2.img
rm -f $OUTDIR/uImage
