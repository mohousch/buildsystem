#!/bin/bash

CURDIR=$1
RELEASEDIR=$2
TMPROOTDIR=$3
TMPKERNELDIR=$4
TMPSTORAGEDIR=$5

find $RELEASEDIR -mindepth 1 -maxdepth 1 -exec cp -at$TMPROOTDIR -- {} +

if [ ! -e $TMPROOTDIR/dev/mtd0 ]; then
	cd $TMPROOTDIR/dev/
	if [ -e $TMPROOTDIR/var/etc/init.d/makedev ]; then
		$TMPROOTDIR/var/etc/init.d/makedev start
	else
		$TMPROOTDIR/etc/init.d/makedev start
	fi
	$TMPROOTDIR/sbin/MAKEDEV ramzswap
	$TMPROOTDIR/sbin/MAKEDEV tundev
	cd -
fi

# --- BOOT ---
mv $TMPROOTDIR/boot/uImage $TMPKERNELDIR/uImage
rm -fr $TMPROOTDIR/boot

# --- VAR ---
mv $TMPROOTDIR/var/* $TMPSTORAGEDIR/

if [ -f $TMPROOTDIR/etc/hostname ]; then
	BOXTYPE=`cat $TMPROOTDIR/etc/hostname`
elif [ -f $TMPSTORAGEDIR/etc/hostname ]; then
	BOXTYPE=`cat $TMPSTORAGEDIR/etc/hostname`
fi

# mini-rcS and inittab
rm -f $TMPROOTDIR/etc
mkdir -p $TMPROOTDIR/etc/init.d
echo "#!/bin/sh" > $TMPROOTDIR/etc/init.d/rcS
echo "mount -n -t proc proc /proc" >> $TMPROOTDIR/etc/init.d/rcS
if [ "$BOXTYPE" == "cuberevo_mini" -o "$BOXTYPE" == "cuberevo_mini2" -o "$BOXTYPE" == "cuberevo" -o "$BOXTYPE" == "cuberevo_2000hd" -o "$BOXTYPE" == "cuberevo_3000hd" ]; then
	echo "mount -t jffs2 -o rw,noatime,nodiratime /dev/mtdblock4 /var" >> $TMPROOTDIR/etc/init.d/rcS
else
	echo "mount -n -t jffs2 -o rw,noatime,nodiratime /dev/mtdblock3 /var" >> $TMPROOTDIR/etc/init.d/rcS
fi
echo "mount --bind /var/etc /etc" >> $TMPROOTDIR/etc/init.d/rcS
echo "/etc/init.d/rcS &" >> $TMPROOTDIR/etc/init.d/rcS
chmod 755 $TMPROOTDIR/etc/init.d/rcS
cp -f $TMPSTORAGEDIR/etc/inittab $TMPROOTDIR/etc
