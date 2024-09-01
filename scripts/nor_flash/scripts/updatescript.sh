cmd=VFD=flash update
cmd=sleep 2
cmd=VFD=loading Image
cmd=usb reset; fatload usb 0:1 a4040000 /kathrein/ufs922/flash.img
cmd=sleep 1
cmd=protect off a0040000 a0ffffff
cmd=VFD=erase wait 3 min
cmd=erase a0040000 a0ffffff
cmd=VFD=flash wait 6 min
cmd=sleep 1
cmd=cp.b a4040000 a0040000 fc0000
cmd=sleep 1
cmd=set bootargs
cmd=set gateway
cmd=set ipaddr 192.168.1.100
cmd=set serverip 192.168.1.1
cmd=set gatewayip 192.168.1.1
cmd=set netmask 255.255.255.0
cmd=set bootargs ''console=ttyAS0,115200 root=/dev/mtdblock2 ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}:kathrein:eth0:off mem=122m init=/bin/devinit coprocessor_mem=4m@0x10000000,4m@0x10400000''
cmd=set bootcmd 'bootm a0040000'
cmd=saveenv
cmd=crc32 a0040000 fc0000
cmd=crc32 a4040000 fc0000
cmd=VFD=ready to go
cmd=sleep 2
cmd=reset
cmd=EOF
