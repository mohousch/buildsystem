#!/bin/bash

CURDIR=$1
TUFSBOXDIR=$2
OUTDIR=$3
TMPROOTDIR=$4
TMPKERNELDIR=$5
TMPFWDIR=$6

DATETIME=_`date +%d.%m.%Y-%H.%M`

CREATEMULTI=$8

echo "CURDIR       = $CURDIR"
echo "TUFSBOXDIR   = $TUFSBOXDIR"
echo "OUTDIR       = $OUTDIR"
echo "TMPROOTDIR   = $TMPROOTDIR"
echo "TMPKERNELDIR = $TMPKERNELDIR"
echo "TMPFWDIR     = $TMPFWDIR"

MKFSJFFS2=$TUFSBOXDIR/host/bin/mkfs.jffs2
SUMTOOL=$TUFSBOXDIR/host/bin/sumtool
MUP=$TUFSBOXDIR/host/bin/mup

if [ -f $TMPROOTDIR/etc/hostname ]; then
	BOXTYPE=`cat $TMPROOTDIR/etc/hostname`
elif [ -f $TMPROOTDIR/var/etc/hostname ]; then
	BOXTYPE=`cat $TMPROOTDIR/var/etc/hostname`
fi

#
OUTFILE=$OUTDIR/update_w_fw.img
OUTFILE_Z=$OUTDIR/$BOXTYPE$DATETIME

if [ ! -e $OUTDIR ]; then
	mkdir $OUTDIR
fi

if [ -e $OUTFILE ]; then
	rm -f $OUTFILE
	rm -f $OUTFILE.md5
fi

cp $TMPKERNELDIR/uImage $OUTDIR/uImage.bin
# Create a jffs2 partition for fw's
# Size 8mb = -p0x800000
# Folder which contains fw's is -r fw
# e.g.
# .
# ./fw
# ./fw/audio.elf
# ./fw/video.elf
$MKFSJFFS2 -qUf -p0x800000 -e0x20000 -r $TMPFWDIR -o $OUTDIR/mtd_fw.bin
$SUMTOOL -p -e 0x20000 -i $OUTDIR/mtd_fw.bin -o $OUTDIR/mtd_fw.sum.bin
rm -f $OUTDIR/mtd_fw.bin
# Create a jffs2 partition for root
# Size 64mb = -p0x4000000
# Folder which contains fw's is -r fw
# e.g.
# .
# ./release
# ./release/etc
# ./release/usr
$MKFSJFFS2 -qUf -p0x4000000 -e0x20000 -r $TMPROOTDIR -o $OUTDIR/mtd_root.bin
$SUMTOOL -p -e 0x20000 -i $OUTDIR/mtd_root.bin -o $OUTDIR/mtd_root.sum.bin
rm -f $OUTDIR/mtd_root.bin
# Create a kathrein update file for fw's
# To get the partitions erased we first need to fake an yaffs2 update
$MUP c $OUTFILE << EOF
2
0x00400000, 0x800000, 3, foo
0x00C00000, 0x4000000, 3, foo
0x00000000, 0x0, 1, $OUTDIR/uImage.bin
0x00400000, 0x0, 1, $OUTDIR/mtd_fw.sum.bin
0x00C00000, 0x0, 1, $OUTDIR/mtd_root.sum.bin
;
EOF

if [ "$CREATEMULTI" == "" ]; then
	md5sum -b $OUTFILE | awk -F' ' '{print $1}' > $OUTFILE.md5
	zip -j $OUTFILE_Z.zip $OUTFILE $OUTFILE.md5
else
	zip -j $OUTFILE_Z.zip $OUTDIR/*.bin
fi
rm -f $OUTFILE
rm -f $OUTFILE.md5
rm -f $OUTDIR/uImage.bin
rm -f $OUTDIR/mtd_fw.sum.bin
rm -f $OUTDIR/mtd_root.sum.bin
