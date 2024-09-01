
===============================================================================
				Software Package - Component 
===============================================================================
	1. readme.txt
	
	2. Release Notes Document
	
	3. driver source code

		3.1 Makefile - to build the modules	   

		3.2 Script and configuration for DHCP:
 	   "wlan0dhcp"
 	   "ifcfg-wlan0"
	   
		3.3 Example of supplicant configuration file:
			"wpa1.conf"

		3.4 Script to run wpa_supplicant
			"runwpa"

	4. wpa_supplicant-0.6.9.tar.gz - the tool help the wlan network to communicate under the 
			protection of WPAPSK mechanism (WPA/WPA2)
	
	5. wpa_supplicant-0.6.9_wps_patch.tar.gz - for wps patch
	
==============================================================================================
			User Guide(1) - connecting wireless networking using "Network Manager" GUI utility
==============================================================================================
			(1) Network Manager is a utility attempts to make use of wireless networking easy.
			
			(2) Notes: if you want to use the following command-line method to connect wireless networking,
									please disable the "Network Manager", because "Network Manager" will conflict with method of command line .



===============================================================================
				User Guide(2) - Set wireless lan MIBs in Command Line
===============================================================================
This driver uses Wireless Extension as an interface allowing you to set
Wireless LAN specific parameters.

Current driver supports "iwlist" to show the device status of nic
        iwlist wlan0 [parameters]
where
        parameter explaination      	[parameters]    
        -----------------------     	-------------   
        Show available chan and freq	freq / channel  
        Show and Scan BSS and IBSS 	scan[ning]          
        Show supported bit-rate         rate / bit[rate]        

For example:
	iwlist wlan0 channel
	iwlist wlan0 scan
	iwlist wlan0 rate

Driver also supports "iwconfig", manipulate driver private ioctls, to set
MIBs.

	iwconfig wlan0 [parameters] [val]
where
	parameter explaination      [parameters]        [val] constraints
        -----------------------     -------------        ------------------
        Connect to AP by address    ap              	[mac_addr]
        Set the essid, join (I)BSS  essid             	[essid]
        Set operation mode          mode                {Managed|Ad-hoc}
        Set keys and security mode  key/enc[ryption]    {N|open|restricted|off}

For example:
	iwconfig wlan0 ap XX:XX:XX:XX:XX:XX
	iwconfig wlan0 essid "ap_name"
	iwconfig wlan0 mode Ad-hoc
	iwconfig wlan0 essid "name" mode Ad-hoc
	iwconfig wlan0 key 0123456789 [2] open
	iwconfig wlan0 key off
	iwconfig wlan0 key restricted [3] 0123456789
        Note: Better to set these MIBS without GUI such as NetworkManager and be sure that our
              nic has been brought up before these settings. WEP key index 2-4 is not supportted by
              NetworkManager.

===============================================================================
				Getting IP address
===============================================================================
After start up the nic, the network needs to obtain an IP address before
transmit/receive data.
This can be done by setting the static IP via "ifconfig wlan0 IP_ADDRESS"
command, or using DHCP.

If using DHCP, setting steps is as below:
	(1)connect to an AP via "iwconfig" settings
		iwconfig wlan0 essid [name]	or
		iwconfig wlan0 ap XX:XX:XX:XX:XX:XX

	(2)run the script which run the dhclient
		./wlan0dhcp
           or 
		dhcpcd wlan0
              	(Some network admins require that you use the
              	hostname and domainname provided by the DHCP server.
              	In that case, use 
		dhcpcd -HD wlan0)
		

===============================================================================
			WPAPSK/WPA2PSK - using wpa_supplicant
===============================================================================
	Wpa_supplicant helps to secure wireless connection with the protection of 
WPAPSK/WPA2PSK mechanism. 

	If the version of Wireless Extension in your system is equal or larger than 18, 
WEXT driver interface is recommended. Otherwise, IPW driver interface is advised.  
	Note: Wireless Extension is defined us "#define WIRELESS_EXT" in Kernel
	Note: To check the version of wireless extension, please type "iwconfig -v"


 	If IPW driver interface is used, it us suggested to follow the steps from 1 to 6. 
If wpa_supplicant has been installed in your system, only steps 5 and 6 are required 
to be executed for WEXT driver interface.

	To see detailed description for driver interface and wpa_supplicant, please type
"man wpa_supplicant".  
	
	(1)Download latetest source code for wpa supplicant or use wpa_supplicant-0.6.9 
	   attached in this package. (It is suggested to use default package contained
           in the distribution because there should less compilation issue.)

	   Unpack source code of WPA supplicant:

	  tar -zxvf wpa_supplicant-0.6.9.tar.gz (e.g.) 
	  cd wpa_supplicant-0.6.9
	
	(2)Create .config file:
	  cp defconfig .config
	
	(3)Edit .config file, uncomment the following line if ipw driver interface 
	   will be applied:
	  #CONFIG_DRIVER_IPW=y.
		
	(4)Build and install WPA supplicant:
	  make
	  cp wpa_cli wpa_supplicant /usr/local/bin	
	
	NOTE:
	 1. If make error for lack of <include/md5.h>, install the openssl lib(two ways):
	  (1) Install the openssl lib from corresponding installation disc:
	      Fedora Core 2/3/4/5(openssl-0.9.71x-xx), 
	      Mandrake10.2/Mandriva10.2(openssl-0.9.7x-xmdk),
	      Debian 3.1(libssl-dev), Suse 9.3/10.0/10.1(openssl_devl), 
	      Gentoo(dev-libs/openssl), etc.
	  (2) Download the openssl open source package from www.openssl.org, build and 
	      install it.
	 2. If make errors happen in RedHat(and also Fedora Core) for kssl.h,
please add lines below into Makefile
	      CPPFLAGS+=-I/usr/kerboros/include
	 
	(5)Edit wpa_supplicant.conf to set up SSID and its passphrase.
	  For example, the following setting in "wpa1.conf" means SSID 
          to join is "BufAG54_Ch6" and its passphrase is "87654321".

	   Example 1: Configuration for WPA-PWK
	  network={
			ssid="BufAG54_Ch6"
			#scan_ssid=1 //see note 3
			proto=WPA
			key_mgmt=WPA-PSK
			pairwise=CCMP TKIP
			group=CCMP TKIP WEP104 WEP40
			psk="87654321"
			priority=2
		  }
	
	    Example 2: Configuration for LEAP
	    network={
			ssid="BufAG54_Ch6"
			key_mgmt=IEEE8021X
			group=WEP40 WEP104
			eap=LEAP
			identity="user1"
			password="1111"
		  }
	    Example 3: Linking to hidden ssid given AP's security policy exactly.(see note 3 below)
            ap_scan=2
	    network={
		ssid="Hidden_ssid"
		proto=WPA
		key_mgmt=WPA-PSK
		pairwise=CCMP
		group=CCMP
		psk="12345678"
	  	}
		
	    Example 4: Linking to ad-hoc (see note 4 below)
	    ap_scan=2
	    network={
		ssid="Ad-hoc"
                mode=1
		proto=WPA
		key_mgmt=WPA-NONE
		pairwise=NONE
		group=TKIP
		psk="12345678"
		}
	Note: 1. proto=WPA for WPA, proto=RSN for WPA2. 
	      2. If user needs to connect an AP with WPA or WPA2 mixed mode, it is suggested 
		 to set the cipher of pairwise and group to both CCMP and TKIP unless you 
		 know exactly which cipher type AP is configured.
	      3. When connecting to hidden ssid, explicit security policy should be given with 
		 ap_scan=2 being setting.
	      4. It is suggested setting ap_scan to 2 and mode to 1 when linking to or creating an ad-hoc. Group and pairwise
		 cipher type should also be explicit, always with group setting to TKIP or CCMP and pairwise setting
		 to NONE. Lower version wpa_supplicant may not allow setting group to CCMP with pairwise setting to NONE.
		 So if any problem, you may try to set both group and pairwise to CCMP, leaving other setting unchanged, when
	         connecting to an CCMP-encrypted ad-hoc.
	      5. More config setting option, please refer to wpa_supplicant.conf in wpa_supplicant.tar.gz that we provide.

	(6)Execute WPA supplicant (Assume driver and related modules had been
           loaded):
           ./runwpa

           Note: The script runwpa will check Wireless Extension version automatically.
                 If the version of Wireless Extension is equal or larger than 18, the
                 option of "-D wext" is selected. If the version of Wireless extension
                 is less than 18, the option of "-D ipw" is selected.

===============================================================================
			WPS - PIN & PBC methods
===============================================================================

		(*) please see the "README-WPS" file in the package "wpa_supplicant-0.6.9.tar.gz"
				to perform the WPS function.
				
===============================================================================
			Power Saving Mode
===============================================================================

		(1) in order to enter PS Mode, you need to add the parameter of "power_mgnt=1" when executing "insmod 8712u.ko" :
		 		~#insmod 8712u.ko power_mgnt=1
		 		or
		 		~#insmod 8712u.ko power_mgnt=2
		 		
		(2) Notes: 
				power_mgnt=0 ;//default, disable PS
		    power_mgnt=1 ;//enable PS, MIN_PS Mode
		    power_mgnt=2 ;//enable PS, MAX_PS Mode
		    
  