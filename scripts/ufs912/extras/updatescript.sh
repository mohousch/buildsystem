cmd=set bootargs 'console=ttyAS0,115200 root=/dev/mtdblock4 rw rootfstype=jffs2 init=/bin/devinit coprocessor_mem=4m@0x40000000,4m@0x40400000 printk=1'
cmd=set bootcmd 'nboot 84000000 0 0;bootm 84000000'
cmd=saveenv
cmd=EOF
