#!/bin/bash

CURDIR=$1
TMPROOTDIR=$2
TMPKERNELDIR=$3
TMPSTORAGEDIR=$4

if [ -f $TMPROOTDIR/etc/hostname ]; then
	BOXTYPE=`cat $TMPROOTDIR/etc/hostname`
elif [ -f $TMPVARDIR/etc/hostname ]; then
	BOXTYPE=`cat $TMPSTORAGEDIR/etc/hostname`
fi

echo "-----------------------------------------------------------------------"
echo "customizing: $BOXTYPE"
# Do your customizations here
rm -f $TMPROOTDIR/lib/modules/cpu_frequ.ko
if [ "$BOXTYPE" == "ufs910" -o "$BOXTYPE" == "ufs922" ];then
	rm -f $TMPROOTDIR/lib/modules/cifs.ko

	rm -f $TMPROOTDIR/usr/bin/curl
	rm -f $TMPROOTDIR/usr/bin/dvbsnoop

	rm -f $TMPROOTDIR/usr/bin/udpxy
	rm -f $TMPROOTDIR/usr/bin/udpxrec

	rm -f $TMPROOTDIR/usr/bin/ntfs-3g
	rm -f $TMPROOTDIR/sbin/mount.ntfs-3g
	rm -f $TMPROOTDIR/sbin/sfdisk
	rm -f $TMPROOTDIR/usr/lib/libntfs-3g.so*
	rm -rf $TMPROOTDIR/usr/lib/ntfs-3g

	find $TMPROOTDIR/usr/share/tuxbox/neutrino/httpd/languages -not -name Deutsch -not -name English -type f -delete
	rm -f $TMPROOTDIR/usr/share/tuxbox/neutrino/icons/dutch.png
	rm -f $TMPROOTDIR/usr/share/tuxbox/neutrino/icons/turkce.png
	find $TMPROOTDIR/usr/share/tuxbox/neutrino/license -not -name deutsch.license -not -name english.license -type f -delete
	find $TMPROOTDIR/usr/share/tuxbox/neutrino/locale -not -name deutsch.locale -not -name english.locale -not -name deutsch.po -not -name english.po -type f -delete
fi

if [ "$BOXTYPE" == "ufs910" -o "$BOXTYPE" == "ufs922" -o "$BOXTYPE" == "fortis_hdbox" -o "$BOXTYPE" == "octagon1008" ];then
	rm -f $TMPROOTDIR/usr/bin/xupnpd
	rm -rf $TMPROOTDIR/usr/share/xupnpd
fi
