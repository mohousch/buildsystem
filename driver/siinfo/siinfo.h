/*
 *  siinfo.h - core box info
 *
 */

#ifndef _SIINFO_H
#define _SIINFO_H
					/*	nims	cicams	cards	avs	usbs	rfmod	fans	*/
					/*--------------------------------------------------------------*/
#define SIINFO_BOXID_55HD	0x0055	/*	1	-	1	-	1	-	-	*/
#define SIINFO_BOXID_99HD	0x0099	/*	2	-	1	1	1	-	1	*/
#define SIINFO_BOXID_9900HD	0x9900	/*	2	2	2	1	2	1	1	*/
					/*--------------------------------------------------------------*/

int siinfo_get_boxid(void);

int siinfo_get_nims(int boxid);
int siinfo_get_cicams(int boxid);
int siinfo_get_creaders(int boxid);
int siinfo_get_avs(int boxid);
int siinfo_get_usbs(int boxid);
int siinfo_get_rfmods(int boxid);
int siinfo_get_fans(int boxid);

#endif
