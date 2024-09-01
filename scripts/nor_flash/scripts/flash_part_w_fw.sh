#!/bin/bash

CURDIR=$1
TUFSBOXDIR=$2
OUTDIR=$3
TMPROOTDIR=$4
TMPKERNELDIR=$5
TMPVARDIR=$6

DATETIME=_`date +%d.%m.%Y-%H.%M`

if [ -f $TMPROOTDIR/etc/hostname ]; then
	BOXTYPE=`cat $TMPROOTDIR/etc/hostname`
elif [ -f $TMPVARDIR/etc/hostname ]; then
	BOXTYPE=`cat $TMPVARDIR/etc/hostname`
fi

echo "CURDIR       = $CURDIR"
echo "TUFSBOXDIR   = $TUFSBOXDIR"
echo "OUTDIR       = $OUTDIR"
echo "TMPROOTDIR   = $TMPROOTDIR"
echo "TMPKERNELDIR = $TMPKERNELDIR"
echo "TMPVARDIR    = $TMPVARDIR"
echo "DATETIME     = $DATETIME"
echo "BOXTYPE      = $BOXTYPE"

MKSQUASHFS=$TUFSBOXDIR/host/bin/mksquashfs
MKFSJFFS2=$TUFSBOXDIR/host/bin/mkfs.jffs2
SUMTOOL=$TUFSBOXDIR/host/bin/sumtool
PAD=$TUFSBOXDIR/host/bin/pad

#
OUTFILE=$OUTDIR/miniFLASH.img
OUTFILE_Z=$OUTDIR/$BOXTYPE$DATETIME

if [ ! -e $OUTDIR ]; then
	mkdir $OUTDIR
fi

if [ -e $OUTFILE ]; then
	rm -f $OUTFILE
	rm -f $OUTFILE.md5
fi

# Definition size of kernel, root, var and erase size
case "$BOXTYPE" in
	ufs910) echo "Creating flash image for $BOXTYPE..."
		SIZE_KERNEL=0x190000
		SIZE_ROOT=0xB40000
		SIZE_VAR=0x2F0000
		ERASE_SIZE=0x10000
	;;
	ufs922) echo "Creating flash image for $BOXTYPE..."
		SIZE_KERNEL=0x1A0000
		SIZE_ROOT=0xB40000
		SIZE_VAR=0x2E0000
		ERASE_SIZE=0x10000
	;;
	fortis_hdbox) echo "Creating flash image for $BOXTYPE..."
		SIZE_KERNEL=0x200000
		SIZE_ROOT=0x11C0000
		SIZE_VAR=0xC00000
		ERASE_SIZE=0x20000
	;;
	octagon1008) echo "Creating flash image for $BOXTYPE..."
		SIZE_KERNEL=0x200000
		SIZE_ROOT=0x11C0000
		SIZE_VAR=0xC00000
		ERASE_SIZE=0x20000
	;;
	cuberevo_mini) echo "Creating flash image for $BOXTYPE..."
		SIZE_KERNEL=0x220000
		SIZE_ROOT=0x1380000
		SIZE_VAR=0xA00000
		ERASE_SIZE=0x20000
		HWMODEL=0x00052000
		HWVERSION=0x00010000
		OUTFILE_OU=$OUTDIR/mtd234.img
		OUTFILE=$OUTDIR/usb_update.img
	;;
	cuberevo_mini2) echo "Creating flash image for $BOXTYPE..."
		SIZE_KERNEL=0x220000
		SIZE_ROOT=0x1380000
		SIZE_VAR=0xA00000
		ERASE_SIZE=0x20000
		HWMODEL=0x00053000
		HWVERSION=0x00010000
		OUTFILE_OU=$OUTDIR/mtd234.img
		OUTFILE=$OUTDIR/usb_update.img
	;;
	cuberevo) echo "Creating flash image for $BOXTYPE..."
		SIZE_KERNEL=0x220000
		SIZE_ROOT=0x1380000
		SIZE_VAR=0xA00000
		ERASE_SIZE=0x20000
		HWMODEL=0x00051100
		HWVERSION=0x00010001
		OUTFILE_OU=$OUTDIR/mtd234.img
		OUTFILE=$OUTDIR/usb_update.img
	;;
	cuberevo_2000hd) echo "Creating flash image for $BOXTYPE..."
		SIZE_KERNEL=0x220000
		SIZE_ROOT=0x1380000
		SIZE_VAR=0xA00000
		ERASE_SIZE=0x20000
		HWMODEL=0x00056000
		HWVERSION=0x00010000
		OUTFILE_OU=$OUTDIR/mtd234.img
		OUTFILE=$OUTDIR/usb_update.img
	;;
	cuberevo_3000hd) echo "Creating flash image for $BOXTYPE..."
		SIZE_KERNEL=0x220000
		SIZE_ROOT=0x1380000
		SIZE_VAR=0xA00000
		ERASE_SIZE=0x20000
		HWMODEL=0x00053000
		HWVERSION=0x00010000
		OUTFILE_OU=$OUTDIR/mtd234.img
		OUTFILE=$OUTDIR/usb_update.img
	;;
	*) echo "Creating flash image for <$BOXTYPE -> ufs910>..."
		SIZE_KERNEL=0x190000
		SIZE_ROOT=0xB40000
		SIZE_VAR=0x2F0000
		ERASE_SIZE=0x10000
	;;
esac

# --- KERNEL ---
cp $TMPKERNELDIR/uImage $CURDIR/uImage
$PAD $SIZE_KERNEL $CURDIR/uImage $CURDIR/mtd_kernel.pad.bin

# --- ROOT ---
# Create a squashfs partition for root
echo "MKSQUASHFS $TMPROOTDIR $CURDIR/mtd_root.bin -noappend -comp gzip -always-use-fragments -b 262144"
$MKSQUASHFS $TMPROOTDIR $CURDIR/mtd_root.bin -noappend -comp gzip -always-use-fragments -b 262144 > /dev/null
echo "PAD $SIZE_ROOT $CURDIR/mtd_root.bin $CURDIR/mtd_root.pad.bin"
$PAD $SIZE_ROOT $CURDIR/mtd_root.bin $CURDIR/mtd_root.pad.bin

# --- VAR ---
# Create a jffs2 partition for var
echo "MKFSJFFS2 -qUf -p$SIZE_VAR -e$ERASE_SIZE -r $TMPVARDIR -o $CURDIR/mtd_var.bin"
$MKFSJFFS2 -qUf -p$SIZE_VAR -e$ERASE_SIZE -r $TMPVARDIR -o $CURDIR/mtd_var.bin
echo "SUMTOOL -p -e $ERASE_SIZE -i $CURDIR/mtd_var.bin -o $CURDIR/mtd_var.sum.bin"
$SUMTOOL -p -e $ERASE_SIZE -i $CURDIR/mtd_var.bin -o $CURDIR/mtd_var.sum.bin
echo "$PAD $SIZE_VAR $CURDIR/mtd_var.sum.bin $CURDIR/mtd_var.sum.pad.bin"
$PAD $SIZE_VAR $CURDIR/mtd_var.sum.bin $CURDIR/mtd_var.sum.pad.bin

# --- update.img ---
#Merge all parts together
if [ "$BOXTYPE" == "cuberevo_mini" -o "$BOXTYPE" == "cuberevo_mini2" -o "$BOXTYPE" == "cuberevo" -o "$BOXTYPE" == "cuberevo_2000hd" -o "$BOXTYPE" == "cuberevo_3000hd" ]; then
	cat $CURDIR/mtd_kernel.pad.bin >> $OUTDIR/out_tmp.img
	cat $CURDIR/mtd_root.pad.bin >> $OUTDIR/out_tmp.img
	cat $CURDIR/mtd_var.sum.pad.bin >> $OUTDIR/out_tmp.img
	cp $OUTDIR/out_tmp.img $OUTFILE_OU
	md5sum -b $OUTFILE_OU | awk -F' ' '{print $1}' > $OUTFILE_OU.md5
	cat $CURDIR/extra/mtd1.img $OUTDIR/out_tmp.img > $OUTDIR/out_tmp1.img
	$CURDIR/extra/mkdnimg -make usbimg -vendor_id 0x00444753 -product_id 0x6c6f6f6b -hw_model $HWMODEL -hw_version $HWVERSION -start_addr 0xa0040000 -erase_size 0x01fc0000 -image_name all_noboot -input $OUTDIR/out_tmp1.img -output $OUTFILE
	rm -f $OUTDIR/out_tmp.img
	rm -f $OUTDIR/out_tmp1.img
else
	cat $CURDIR/mtd_kernel.pad.bin >> $OUTFILE
	cat $CURDIR/mtd_root.pad.bin >> $OUTFILE
	cat $CURDIR/mtd_var.sum.pad.bin >> $OUTFILE
fi

echo "-----------------------------------------------------------------------"
echo "flash size results for : $BOXTYPE"
SIZE=`stat uImage -t --format %s`
SIZE=`printf "0x%X" $SIZE`
echo -e "\e[32mKernel = $SIZE Maximum= $SIZE_KERNEL\e[0m"

SIZE=`stat mtd_var.sum.bin -t --format %s`
SIZE=`printf "0x%X" $SIZE`
echo -e "\e[32mVar    = $SIZE Maximum= $SIZE_VAR\e[0m"

SIZE=`stat mtd_root.bin -t --format %s`
SIZE=`printf "0x%X" $SIZE`
echo -e "\e[32mRoot   = $SIZE Maximum= $SIZE_ROOT\e[0m"

rm -f $CURDIR/uImage
rm -f $CURDIR/mtd_root.bin
rm -f $CURDIR/mtd_var.bin
rm -f $CURDIR/mtd_var.sum.bin

SIZE=`stat mtd_kernel.pad.bin -t --format %s`
SIZE=`printf "0x%X" $SIZE`
if [[ $SIZE > "$SIZE_KERNEL" ]]; then
	echo -e "\e[31mKERNEL TO BIG. $SIZE instead of $SIZE_KERNEL\e[0m" > /dev/stderr
	read -p "Press ENTER to continue..."
fi

SIZE=`stat mtd_root.pad.bin -t --format %s`
SIZE=`printf "0x%X" $SIZE`
if [[ $SIZE > "$SIZE_ROOT" ]]; then
	echo -e "\e[31mROOT TO BIG. $SIZE instead of $SIZE_ROOT\e[0m" > /dev/stderr
	read -p "Press ENTER to continue..."
fi

SIZE=`stat mtd_var.sum.pad.bin -t --format %s`
SIZE=`printf "0x%X" $SIZE`
if [[ $SIZE > "$SIZE_VAR" ]]; then
	echo -e "\e[31mVAR TO BIG. $SIZE instead of $SIZE_VAR\e[0m" > /dev/stderr
	read -p "Press ENTER to continue..."
fi

rm -f $CURDIR/mtd_kernel.pad.bin
rm -f $CURDIR/mtd_root.pad.bin
rm -f $CURDIR/mtd_var.sum.pad.bin

md5sum -b $OUTFILE | awk -F' ' '{print $1}' > $OUTFILE.md5
if [ "$BOXTYPE" == "cuberevo_mini" -o "$BOXTYPE" == "cuberevo_mini2" -o "$BOXTYPE" == "cuberevo" -o "$BOXTYPE" == "cuberevo_2000hd" -o "$BOXTYPE" == "cuberevo_3000hd" ]; then
	zip -j $OUTFILE_Z.zip $OUTFILE $OUTFILE.md5 $OUTFILE_OU $OUTFILE_OU.md5
	rm -f $OUTFILE_OU
	rm -f $OUTFILE_OU.md5
else
	zip -j $OUTFILE_Z.zip $OUTFILE $OUTFILE.md5
fi
rm -f $OUTFILE
rm -f $OUTFILE.md5
