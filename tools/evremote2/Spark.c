/*
 * Spark.c
 *
 * (c) 2010 duckbox project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <termios.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>

#include "global.h"
#include "map.h"
#include "remotes.h"
#include "Spark.h"


#define	SPARK_RC04_PREDATA      "CC33"
#define	SPARK_RC05_PREDATA      "11EE"
#define	SPARK_RC08_PREDATA      "44BB"
#define	SPARK_RC09_PREDATA      "9966"
#define	SPARK_RC12_PREDATA      "08F7"
#define	SPARK_DEFAULT_PREDATA   "A25D" //HOF-12, SAB Unix Triple HD, Sogno Triple
#define	SPARK_EDV_RC1           "C43B"
#define	SPARK_EDV_RC2           "1CE3"
#define	UFS910_RC660_PREDATA    "2290"
#define	UFS913_RC230_PREDATA    "7FB9"
#define	SAMSUNG_AA59_PREDATA    "E0E0"

#define STB_ID_GOLDENMEDIA_GM990        "09:00:07"
#define STB_ID_EDISION_PINGULUX         "09:00:08"
#define STB_ID_AMIKO_ALIEN_SDH8900      "09:00:0A"
#define STB_ID_GALAXYINNOVATIONS_S8120  "09:00:0B"
#define STB_ID_SOGNO_TRIPLE_HD          "0C:00:43" // Sogno Spark Triple
#define STB_ID_SAB_UNIX_TRIPLE_HD       "0C:00:0E" // SAB Unix Triple HD (S903)

char VendorStbId[] = "00:00:00\0";

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 120,
};

/* Edision argus-spark RCU */
static tButton cButtonsEdisionSpark[] =
{
	{"STANDBY"        , "25", KEY_POWER},
	{"MUTE"           , "85", KEY_MUTE},
	{"V.FORMAT"       , "ad", KEY_V},
	{"TV/SAT"         , "c5", KEY_AUX},
	{"0BUTTON"        , "57", KEY_0},
	{"1BUTTON"        , "b5", KEY_1},
	{"2BUTTON"        , "95", KEY_2},
	{"3BUTTON"        , "bd", KEY_3},
	{"4BUTTON"        , "f5", KEY_4},
	{"5BUTTON"        , "d5", KEY_5},
	{"6BUTTON"        , "fd", KEY_6},
	{"7BUTTON"        , "35", KEY_7},
	{"8BUTTON"        , "15", KEY_8},
	{"9BUTTON"        , "3d", KEY_9},
	{"BACK"           , "7f", KEY_BACK},
	{"INFO"           , "a7", KEY_INFO}, //THIS IS WRONG SHOULD BE KEY_INFO
	{"AUDIO"          , "35", KEY_AUDIO},
	{"VOL+"           , "C7", KEY_VOLUMEUP},
	{"VOL-"           , "DD", KEY_VOLUMEDOWN},
	{"PAGE+"          , "07", KEY_PAGEUP},
	{"PAGE-"          , "5F", KEY_PAGEDOWN},
	{"DOWN"           , "0f", KEY_DOWN},
	{"UP"             , "27", KEY_UP},
	{"RIGHT"          , "af", KEY_RIGHT},
	{"LEFT"           , "6d", KEY_LEFT},
	{"OK/LIST"        , "2f", KEY_OK},
	{"MENU"           , "65", KEY_MENU},
	{"GUIDE"          , "8f", KEY_EPG},
	{"EXIT"           , "4d", KEY_HOME},
	{"FAV"            , "87", KEY_FAVORITES},
	{"RED"            , "7d", KEY_RED},
	{"GREEN"          , "ff", KEY_GREEN},
	{"YELLOW"         , "3f", KEY_YELLOW},
	{"BLUE"           , "bf", KEY_BLUE},
	{"REWIND"         , "1f", KEY_REWIND},
	{"PAUSE"          , "37", KEY_PAUSE},
	{"PLAY"           , "b7", KEY_PLAY},
	{"FASTFORWARD"    , "97", KEY_FASTFORWARD},
	{"RECORD"         , "45", KEY_RECORD},
	{"STOP"           , "f7", KEY_STOP},
	{"SLOWMOTION"     , "5d", KEY_SLOW},
	{"ARCHIVE"        , "75", KEY_ARCHIVE},
	{"SAT"            , "1d", KEY_SAT},
	{"STEPBACK"       , "55", KEY_PREVIOUS},
	{"STEPFORWARD"    , "d7", KEY_NEXT},
	{"MARK"           , "8f", KEY_EPG},
	{"TV/RADIO"       , "77", KEY_TV2}, //WE USE TV2 AS TV/RADIO SWITCH BUTTON
	{"USB"            , "95", KEY_CLOSE},
	{"TIMER"          , "8d", KEY_TIME},
	{""               , ""  , KEY_NULL}
};

/* spark RC8 */
static tButton cButtonsSparkRc08[] =
{
	{"POWER"          , "4D", KEY_POWER},
	{"MUTE"           , "DD", KEY_MUTE},
	{"TIME"           , "ED", KEY_TIME},
	{"V.FORMAT"       , "AD", KEY_V},
	{"F1"             , "0F", KEY_F1},
	{"TV/SAT"         , "37", KEY_AUX},
	{"USB"            , "0D", KEY_CLOSE},
	{"FIND"           , "35", KEY_FIND},
	{"0BUTTON"        , "27", KEY_0},
	{"1BUTTON"        , "7D", KEY_1},
	{"2BUTTON"        , "3F", KEY_2},
	{"3BUTTON"        , "BD", KEY_3},
	{"4BUTTON"        , "5D", KEY_4},
	{"5BUTTON"        , "1F", KEY_5},
	{"6BUTTON"        , "9D", KEY_6},
	{"7BUTTON"        , "55", KEY_7},
	{"8BUTTON"        , "17", KEY_8},
	{"9BUTTON"        , "95", KEY_9},
	{"TV/RADIO"       , "65", KEY_TV2}, //WE USE TV2 AS TV/RADIO SWITCHB
	{"RECALL"         , "A5", KEY_BACK},
	{"MENU"           , "5F", KEY_MENU},
	{"INFO"           , "1D", KEY_INFO},
	{"UP"             , "9F", KEY_UP},
	{"DOWN"           , "AF", KEY_DOWN},
	{"LEFT"           , "3D", KEY_LEFT},
	{"RIGHT"          , "7F", KEY_RIGHT},
	{"OK"             , "BF", KEY_OK},
	{"EXIT"           , "2D", KEY_HOME},
	{"EPG"            , "25", KEY_EPG},
	{"VOL+"           , "8D", KEY_VOLUMEUP},
	{"VOL-"           , "B5", KEY_VOLUMEDOWN},
	{"REC"            , "EF", KEY_RECORD},
	{"PAGE+"          , "B7", KEY_PAGEUP},
	{"PAGE-"          , "77", KEY_PAGEDOWN},
	{"FOLDER"         , "E5", KEY_ARCHIVE},
	{"STOP"           , "A7", KEY_STOP},
	{"PLAY"           , "75", KEY_PLAY},
	{"PAUSE"          , "F5", KEY_PAUSE},
	{"FASTFORWARD"    , "CD", KEY_FASTFORWARD},
	{"REWIND"         , "D5", KEY_REWIND},
	{"PREV"           , "8F", KEY_PREVIOUS},
	{"NEXT"           , "57", KEY_NEXT},
	{"Tms"            , "4F", KEY_T},
	{"FAST"           , "97", KEY_F},
	{"SLOW"           , "15", KEY_SLOW},
	{"PLAY_MODE"      , "6F", KEY_P},
	{"WHITE"          , "67", KEY_W},
	{"RED"            , "05", KEY_RED},
	{"GREEN"          , "87", KEY_GREEN},
	{"YELLOW"         , "C5", KEY_YELLOW},
	{"BLUE"           , "47", KEY_BLUE},
	{"FAV"            , "F7", KEY_FAVORITES},
	{"SAT"            , "2F", KEY_SAT},
	{"TTX"            , "DF", KEY_TITLE},
	{"AUDIO"          , "D7", KEY_SUBTITLE},
	{""               , ""  , KEY_NULL}
};

/* Amiko alien-spark RCU */
static tButton cButtonsSparkRc09[] =
{
	{"POWER"          , "25", KEY_POWER},
	{"MUTE"           , "85", KEY_MUTE},
	{"TIME"           , "8D", KEY_TIME},
	{"V.FORMAT"       , "AD", KEY_V},
	{"TV/SAT"         , "A5", KEY_AUX},
	{"PICASA"         , "E5", KEY_SUBTITLE},
	{"SHOUTCAST"      , "ED", KEY_AUDIO},
	{"YOUTUBE"        , "CD", KEY_VIDEO},
	{"SPARK"          , "C5", KEY_S},
	{"0BUTTON"        , "57", KEY_0},
	{"1BUTTON"        , "B5", KEY_1},
	{"2BUTTON"        , "95", KEY_2},
	{"3BUTTON"        , "BD", KEY_3},
	{"4BUTTON"        , "F5", KEY_4},
	{"5BUTTON"        , "D5", KEY_5},
	{"6BUTTON"        , "FD", KEY_6},
	{"7BUTTON"        , "35", KEY_7},
	{"8BUTTON"        , "15", KEY_8},
	{"9BUTTON"        , "3D", KEY_9},
	{"TV/RADIO"       , "77", KEY_TV2}, //WE USE TV2 AS TV/RADIO SWITCHB
	{"RECALL"         , "7F", KEY_BACK},
	{"SAT"            , "9D", KEY_SAT},
	{"FAV"     	      , "45", KEY_FAVORITES},
	{"VOL-"           , "C7", KEY_VOLUMEDOWN},
	{"VOL+"           , "DD", KEY_VOLUMEUP},
	{"PAGE-"          , "07", KEY_CHANNELDOWN},
	{"PAGE+"          , "5F", KEY_CHANNELUP},
	{"INFO"           , "1D", KEY_INFO},
	{"EPG"            , "87", KEY_EPG},
	{"MENU"           , "65", KEY_MENU},
	{"EXIT"           , "A7", KEY_HOME},
	{"UP"	      , "27", KEY_UP},
	{"DOWN"	      , "0F", KEY_DOWN},
	{"LEFT" 	      , "6D", KEY_LEFT},
	{"RIGHT"	      , "AF", KEY_RIGHT},
	{"OK"	      , "2F", KEY_OK},
	{"FIND"           , "4D", KEY_FIND},
	{"REC"            , "8F", KEY_RECORD},
	{"RED"            , "75", KEY_RED},
	{"GREEN"          , "F7", KEY_GREEN},
	{"YELLOW"         , "37", KEY_YELLOW},
	{"BLUE"           , "B7", KEY_BLUE},
	{"REWIND"	      , "55", KEY_REWIND},
	{"PLAY" 	      , "D7", KEY_PLAY},
	{"PAUSE"	      , "17", KEY_PAUSE},
	{"FASTFORWARD"    , "97", KEY_FASTFORWARD},
	{"FOLDER"	      , "5D", KEY_ARCHIVE},
	{"PLAY_MODE"      , "DF", KEY_P},
	{"USB"            , "1F", KEY_CLOSE},
	{"STOP" 	      , "9F", KEY_STOP},
	{"F1"             , "7D", KEY_HELP},
	{"F2"             , "FF", KEY_PREVIOUS},
	{"F3"             , "3F", KEY_NEXT},
	{"F4"             , "BF", KEY_TITLE},
	{""               , ""  , KEY_NULL}
};

static tButton cButtonsSparkRc12[] =
{
	{"MUTE"           , "87", KEY_MUTE},
	{"POWER"          , "45", KEY_POWER},
	{"PLAY_MODE"      , "A7", KEY_P},
	{"V.FORMAT"       , "E5", KEY_V},
	{"TIME"           , "C5", KEY_TIME},
	{"USB"            , "47", KEY_CLOSE},
	{"FOLDER"         , "65", KEY_ARCHIVE},
	{"STOP"           , "25", KEY_STOP},
	{"PLAY"           , "3D", KEY_PLAY},
	{"PAUSE"          , "1D", KEY_PAUSE},
	{"FASTFORWARD"    , "C7", KEY_FASTFORWARD},
	{"REWIND"         , "FD", KEY_REWIND},
	{"PREV"           , "BF", KEY_PREVIOUS},
	{"NEXT"           , "E7", KEY_NEXT},
	{"FAST"           , "67", KEY_F},
	{"SLOW"           , "9F", KEY_SLOW},
	{"MENU"           , "DD", KEY_MENU},
	{"Tms"            , "BD", KEY_T},
	{"INFO"           , "FF", KEY_INFO},
	{"UP"	      	  , "5D", KEY_UP},
	{"DOWN"           , "55", KEY_DOWN},
	{"LEFT"           , "1F", KEY_LEFT},
	{"RIGHT"          , "7F", KEY_RIGHT},
	{"OK"             , "7D", KEY_OK},
	{"EXIT"           , "3F", KEY_HOME},
	{"REC"            , "9D", KEY_RECORD},
	{"EPG"            , "5F", KEY_EPG},
	{"TV/SAT"         , "D5", KEY_AUX},
	{"RECALL"         , "DF", KEY_BACK},
	{"FIND"           , "95", KEY_FIND},
	{"VOL+"           , "17", KEY_VOLUMEUP},
	{"VOL-"           , "37", KEY_VOLUMEDOWN},
	{"SAT"            , "15", KEY_SAT},
	{"FAV"            , "35", KEY_FAVORITES},
	{"PAGE+"          , "57", KEY_PAGEUP},
	{"PAGE-"          , "D7", KEY_PAGEDOWN},
	{"KEY_0"          , "0D", KEY_0},
	{"KEY_1"          , "F5", KEY_1},
	{"KEY_2"          , "B5", KEY_2},
	{"KEY_3"          , "F7", KEY_3},
	{"KEY_4"          , "CD", KEY_4},
	{"KEY_5"          , "AD", KEY_5},
	{"KEY_6"          , "77", KEY_6},
	{"KEY_7"          , "0F", KEY_7},
	{"KEY_8"          , "8D", KEY_8},
	{"KEY_9"          , "4F", KEY_9},
	{"TV/RADIO"       , "ED", KEY_TV2},
	{"RED"            , "2F", KEY_RED},
	{"GREEN"          , "6F", KEY_GREEN},
	{"YELLOW"         , "EF", KEY_YELLOW},
	{"BLUE"           , "05", KEY_BLUE},
	{"WHITE"          , "2D", KEY_W},
	{""               , ""  , KEY_NULL}
};

/* spark Default */
static tButton cButtonsSparkDefault[] =
{
	{"KEY_POWER"		, "87", KEY_POWER},
	{"KEY_MODE"		, "0F", KEY_MODE},
	{"KEY_TEXT"		, "2F", KEY_TEXT},			//TV/SAT
	{"KEY_SLEEP"		, "65", KEY_SLEEP},
	{"KEY_MUTE"		, "A5", KEY_MUTE},
	{"KEY_0"		, "37", KEY_0},
	{"KEY_1"		, "A7", KEY_1},
	{"KEY_2"		, "07", KEY_2},
	{"KEY_3"		, "E5", KEY_3},
	{"KEY_4"		, "97", KEY_4},
	{"KEY_5"		, "27", KEY_5},
	{"KEY_6"		, "D5", KEY_6},
	{"KEY_7"		, "B7", KEY_7},
	{"KEY_8"		, "17", KEY_8},
	{"KEY_9"		, "F5", KEY_9},
	{"KEY_RADIO"		, "CD", KEY_RADIO},			//WE USE TV2 AS TV/RADIO SWITCHB
	{"KEY_BACK"		, "CF", KEY_BACK},			//RECALL
	{"KEY_PAGEUP"		, "ED", KEY_PAGEUP},
	{"KEY_PAGEDOWN"		, "DD", KEY_PAGEDOWN},
	{"KEY_SEARCH"		, "C5", KEY_SEARCH},
	{"KEY_MEDIA"		, "67", KEY_MEDIA},			//FOLDER
	{"KEY_VOLUMEUP"		, "AF", KEY_VOLUMEUP},
	{"KEY_VOLUMEDOWN"	, "9F", KEY_VOLUMEDOWN},
	{"KEY_MENU"		, "C7", KEY_MENU},
	{"KEY_INFO"		, "1F", KEY_INFO},
	{"KEY_UP"		, "3F", KEY_UP},
	{"KEY_DOWN"		, "85", KEY_DOWN},
	{"KEY_LEFT"		, "BF", KEY_LEFT},
	{"KEY_RIGHT"		, "FD", KEY_RIGHT},
	{"KEY_OK"		, "05", KEY_OK},
	{"KEY_HOME"		, "F9", KEY_HOME},
	{"KEY_EPG"		, "45", KEY_EPG},
	{"KEY_FAVORITES"	, "3D", KEY_FAVORITES},
	{"KEY_EPG"		, "25", KEY_EPG},			// Portal
	{"KEY_SAT"		, "0D", KEY_SAT},
	{"KEY_RED"		, "6D", KEY_RED},
	{"KEY_GREEN"		, "8D", KEY_GREEN},
	{"KEY_YELLOW"		, "77", KEY_YELLOW},
	{"KEY_BLUE"		, "AD", KEY_BLUE},
	{"KEY_RECORD"		, "F7", KEY_RECORD},
	{"KEY_STOP"		, "BB", KEY_STOP},
	{"KEY_PLAY"		, "57", KEY_PLAY},
	{"KEY_PAUSE"		, "4D", KEY_PAUSE},
	{"KEY_FASTFORWARD"	, "35", KEY_FASTFORWARD},
	{"KEY_REWIND"		, "7F", KEY_REWIND},
	{"KEY_PREVIOUS"		, "FB", KEY_PREVIOUS},
	{"KEY_NEXT"		, "3B", KEY_NEXT},
	{"KEY_FAST"		, "E7", KEY_F},
	{"KEY_SLOW"		, "7B", KEY_SLOW},
	{"KEY_PLAYPAUSE"	, "B5", KEY_PLAYPAUSE},
	{"KEY_CLOSE"		, "DF", KEY_CLOSE},			//USB
	{"KEY_TIME"		, "55", KEY_TIME},
	{"KEY_AUDIO"		, "95", KEY_AUDIO},			//F1
	{"KEY_SUBTITLE"		, "15", KEY_SUBTITLE},			//F2
	{"KEY_LIST"		, "D7", KEY_LIST},			//F3
	{""			, ""  , KEY_NULL}
};

static tButton cButtonsSognoTriplex[] =
{
	{"KEY_POWER"		, "10", KEY_POWER},			//# Power
	{"KEY_MODE"		, "01", KEY_MODE},			//# V.Format
	{"KEY_TEXT"		, "05", KEY_TEXT},			//# TV/SAT
	{"KEY_SLEEP"		, "4C", KEY_SLEEP},			//# SLEEP
	{"KEY_MUTE"		, "54", KEY_MUTE},			//# MUTE
	{"KEY_1"		, "14", KEY_1},				//# 1
	{"KEY_2"		, "00", KEY_2},				//# 2
	{"KEY_3"		, "5C", KEY_3},				//# 3
	{"KEY_4"		, "12", KEY_4},				//# 4
	{"KEY_5"		, "04", KEY_5},				//# 5
	{"KEY_6"		, "5A", KEY_6},				//# 6
	{"KEY_7"		, "16", KEY_7},				//# 7
	{"KEY_8"		, "02", KEY_8},				//# 8
	{"KEY_9"		, "5E", KEY_9},				//# 9
	{"KEY_RADIO"		, "59", KEY_RADIO},			//# TV/Radio
	{"KEY_0"		, "06", KEY_0},				//# 0
	{"KEY_BACK"		, "19", KEY_BACK},			//# Recall
	{"KEY_PAGEUP"		, "5D", KEY_PAGEUP},			//# PAGE+
	{"KEY_PAGEDOWN"		, "5B", KEY_PAGEDOWN},			//# PAGE-
	{"KEY_SEARCH"		, "58", KEY_SEARCH},			//# FIND
	{"KEY_VOLUMEUP"		, "15", KEY_VOLUMEUP},			//# VOL+
	{"KEY_VOLUMEDOWN"	, "13", KEY_VOLUMEDOWN},		//# VOL-
	{"KEY_MEDIA"		, "0C", KEY_MEDIA},			//# FOLDER
	{"KEY_MENU"		, "18", KEY_MENU},			//# MENU
	{"KEY_INFO"		, "03", KEY_INFO},			//# INFO
	{"KEY_UP"		, "07", KEY_UP},			//# UP
	{"KEY_LEFT"		, "17", KEY_LEFT},			//# LEFT
	{"KEY_RIGHT"		, "5F", KEY_RIGHT},			//# RIGHT
	{"KEY_DOWN"		, "50", KEY_DOWN},			//# DOWN
	{"KEY_OK"		, "40", KEY_OK},			//# OK
	{"KEY_HOME"		, "DF", KEY_HOME},			//# EXIT
	{"KEY_EPG"		, "48", KEY_EPG},			//# EPG
	{"KEY_FAVORITES"	, "47", KEY_FAVORITES},			//# FAV
	{"KEY_EPG"		, "44", KEY_EPG},			//# Portal
	{"KEY_SAT"		, "41", KEY_SAT},			//# SAT
	{"KEY_RED"		, "4D", KEY_RED},			//# RED
	{"KEY_GREEN"		, "51", KEY_GREEN},			//# GREEN
	{"KEY_YELLOW"		, "0E", KEY_YELLOW},			//# YELLOW
	{"KEY_BLUE"		, "55", KEY_BLUE},			//# BLUE
	{"KEY_RECORD"		, "1E", KEY_RECORD},			//# RECORD
	{"KEY_STOP"		, "97", KEY_STOP},			//# STOP
	{"KEY_PAUSE"		, "49", KEY_PAUSE},			//# PAUSE
	{"KEY_PLAY"		, "0A", KEY_PLAY},			//# PLAY
	{"KEY_PREVIOUS"		, "9F", KEY_PREVIOUS},			//# PREVIOUS
	{"KEY_NEXT"		, "87", KEY_NEXT},			//# NEXT
	{"KEY_PAGEDOWN"		, "0F", KEY_PAGEDOWN},			//# REWIND
	{"KEY_PAGEUP"		, "46", KEY_PAGEUP},			//# FORWARD
	{"KEY_REWIND"		, "8F", KEY_REWIND},			//# SLOW
	{"KEY_FASTFORWARD"	, "1C", KEY_FASTFORWARD},		//# FASTFORWARD
	{"KEY_PLAYPAUSE"	, "56", KEY_PLAYPAUSE},			//# PLAYMODE
	{"KEY_CLOSE"		, "1B", KEY_CLOSE},			//# USB
	{"KEY_TIME"		, "4A", KEY_TIME},			//# Timeshift
	{"KEY_AUDIO"		, "52", KEY_AUDIO},			//# F1
	{"KEY_SUBTITLE"		, "42", KEY_SUBTITLE},			//# F2
	{"KEY_LIST"		, "1A", KEY_LIST},			//# F3
	{""			, ""  , KEY_NULL}
};

static tButton cButtonsUfs910Rc660[] =
{
	{"KEY_0"		, "00", KEY_0},
	{"KEY_1"		, "01", KEY_1},
	{"KEY_2"		, "02", KEY_2},
	{"KEY_3"		, "03", KEY_3},
	{"KEY_4"		, "04", KEY_4},
	{"KEY_5"		, "05", KEY_5},
	{"KEY_6"		, "06", KEY_6},
	{"KEY_7"		, "07", KEY_7},
	{"KEY_8"		, "08", KEY_8},
	{"KEY_9"		, "09", KEY_9},
	{"KEY_INFO"		, "0F", KEY_INFO},
	{"KEY_OK"		, "5C", KEY_OK},
	{"KEY_POWER"		, "0C", KEY_POWER},
	{"KEY_MUTE"		, "0D", KEY_MUTE},
	{"KEY_RIGHT"		, "5B", KEY_RIGHT},
	{"KEY_LEFT"		, "5A", KEY_LEFT},
	{"KEY_UP"		, "58", KEY_UP},
	{"KEY_DOWN"		, "59", KEY_DOWN},
	{"KEY_VOLUMEUP"		, "10", KEY_VOLUMEUP},
	{"KEY_VOLUMEDOWN"	, "11", KEY_VOLUMEDOWN},
	{"KEY_RED"		, "6D", KEY_RED},
	{"KEY_GREEN"		, "6E", KEY_GREEN},
	{"KEY_YELLOW"		, "6F", KEY_YELLOW},
	{"KEY_BLUE"		, "70", KEY_BLUE},
	{"KEY_EPG"		, "CC", KEY_EPG},
	{"KEY_HOME"		, "55", KEY_HOME},
	{"KEY_MENU"		, "54", KEY_MENU},
	{"KEY_PAGEUP"		, "1E", KEY_PAGEUP},
	{"KEY_PAGEDOWN"		, "1F", KEY_PAGEDOWN},
	{"KEY_PLAY"		, "38", KEY_PLAY},
	{"KEY_STOP"		, "31", KEY_STOP},
	{"KEY_RECORD"		, "37", KEY_RECORD},
	{"KEY_PAUSE"		, "39", KEY_PAUSE},
	{"KEY_FASTFORWARD"	, "20", KEY_FASTFORWARD},
	{"KEY_REWIND"		, "21", KEY_REWIND},
	{"KEY_TEXT"		, "3C", KEY_TEXT},
	{""			, ""  , KEY_NULL}
};

static tButton cButtonsSamsungAA59[] =
{
	{"KEY_POWER"		, "BF", KEY_POWER},
	{"KEY_MODE"		, "7F", KEY_MODE},		//# SOURCE
	{"KEY_SLEEP"		, "2E", KEY_SLEEP},		//# HDMI
	{"KEY_1"		, "DF", KEY_1},
	{"KEY_2"		, "5F", KEY_2},
	{"KEY_3"		, "9F", KEY_3},
	{"KEY_4"		, "EF", KEY_4},
	{"KEY_5"		, "6F", KEY_5},
	{"KEY_6"		, "AF", KEY_6},
	{"KEY_7"		, "CF", KEY_7},
	{"KEY_8"		, "4F", KEY_8},
	{"KEY_9"		, "8F", KEY_9},
	{"KEY_0"		, "77", KEY_0},
	{"KEY_VOLUMEUP"		, "1F", KEY_VOLUMEUP},
	{"KEY_VOLUMEDOWN"	, "2F", KEY_VOLUMEDOWN},
	{"KEY_PAGEUP"		, "B7", KEY_PAGEUP},
	{"KEY_PAGEDOWN"		, "F7", KEY_PAGEDOWN},
	{"KEY_MUTE"		, "0F", KEY_MUTE},
	{"KEY_TEXT"		, "29", KEY_TEXT},		//# CKLIST
	{"KEY_MENU"		, "A7", KEY_MENU},
	{"KEY_smart"		, "61", KEY_ARCHIVE},		//# SMART
	{"KEY_EPG"		, "0D", KEY_EPG},		//# GUIDE
	{"KEY_FAVORITES"	, "2D", KEY_FAVORITES},		//# TOOLS
	{"KEY_INFO"		, "07", KEY_INFO},
	{"KEY_UP"		, "F9", KEY_UP},
	{"KEY_DOWN"		, "79", KEY_DOWN},
	{"KEY_LEFT"		, "59", KEY_LEFT},
	{"KEY_RIGHT"		, "B9", KEY_RIGHT},
	{"KEY_OK"		, "E9", KEY_OK},
	{"KEY_OK"		, "E5", KEY_OK},		//# RETURN
	{"KEY_HOME"		, "4B", KEY_HOME},		//# EXIT
	{"KEY_RED"		, "C9", KEY_RED},
	{"KEY_GREEN"		, "D7", KEY_GREEN},
	{"KEY_YELLOW"		, "57", KEY_YELLOW},
	{"KEY_BLUE"		, "97", KEY_BLUE},
	{"KEY_AUDIO"		, "D9", KEY_AUDIO},		//# SOCIAL
	{"search"		, "31", KEY_TIME},		//# SEARCH
	{"3d"			, "06", KEY_LIST},		//# 3D
	{"manual"		, "03", KEY_PREVIOUS},		//# MANUAL
	{"size"			, "83", KEY_NEXT},		//# SIZE
	{"subt"			, "5B", KEY_SUBTITLE},
	{"KEY_REWIND"		, "5D", KEY_REWIND},
	{"KEY_PAUSE"		, "AD", KEY_PAUSE},
	{"KEY_FASTFORWARD"	, "ED", KEY_FASTFORWARD},
	{"KEY_RECORD"		, "6D", KEY_RECORD},
	{"KEY_PLAY"		, "1D", KEY_PLAY},
	{"KEY_STOP"		, "9D", KEY_STOP},
	{""			, ""  , KEY_NULL}
};


static tButton cButtonsUfs913Rc230[] =
{
	{"KEY_MODE"		, "7E", KEY_MODE},
	{"KEY_POWER"		, "F3", KEY_POWER},
	{"KEY_1"		, "FE", KEY_1},
	{"KEY_2"		, "FD", KEY_2},
	{"KEY_3"		, "FC", KEY_3},
	{"KEY_4"		, "FB", KEY_4},
	{"KEY_5"		, "FA", KEY_5},
	{"KEY_6"		, "F9", KEY_6},
	{"KEY_7"		, "F8", KEY_7},
	{"KEY_8"		, "F7", KEY_8},
	{"KEY_9"		, "F6", KEY_9},
	{"KEY_0"		, "FF", KEY_0},
	{"KEY_VOLUMEDOWN"	, "EE", KEY_VOLUMEDOWN},
	{"KEY_VOLUMEUP"		, "EF", KEY_VOLUMEUP},
	{"KEY_PAGEUP"		, "E1", KEY_PAGEUP},
	{"KEY_PAGEDOWN"		, "E0", KEY_PAGEDOWN},
	{"KEY_OK"		, "A3", KEY_OK},
	{"KEY_MENU"		, "AB", KEY_MENU},
	{"KEY_TEXT"		, "C3", KEY_TEXT},
	{"KEY_MUTE"		, "F2", KEY_MUTE},
	{"KEY_INFO"		, "F0", KEY_INFO},
	{"KEY_RED"		, "92", KEY_RED},
	{"KEY_GREEN"		, "91", KEY_GREEN},
	{"KEY_YELLOW"		, "90", KEY_YELLOW},
	{"KEY_BLUE"		, "8F", KEY_BLUE},
	{"KEY_EPG"		, "33", KEY_EPG},
	{"KEY_MEDIA"		, "B9", KEY_MEDIA},
	{"KEY_UP"		, "A7", KEY_UP},
	{"KEY_DOWN"		, "A6", KEY_DOWN},
	{"KEY_LEFT"		, "A5", KEY_LEFT},
	{"KEY_RIGHT"		, "A4", KEY_RIGHT},
	{"KEY_HOME"		, "AA", KEY_HOME},
	{"KEY_FAVORITES"	, "2A", KEY_FAVORITES}, //Portal
	{"KEY_REWIND"		, "DE", KEY_REWIND},
	{"KEY_FASTFORWARD"	, "DF", KEY_FASTFORWARD},
	{"KEY_PLAY"		, "C7", KEY_PLAY},
	{"KEY_PAUSE"		, "C6", KEY_PAUSE},
	{"KEY_RECORD"		, "C8", KEY_RECORD},
	{"KEY_STOP"		, "CE", KEY_STOP},
	{""			, ""  , KEY_NULL}
};

/* spark Default */
static tButton cButtonsSparkRc04[] =
{
	{"POWER"          , "45", KEY_POWER},
	{"V.FORMAT"       , "4D", KEY_V},
	{"TIME"           , "ED", KEY_TIME},
	{"MUTE"           , "8D", KEY_MUTE},
	{"Tms"            , "57", KEY_T},
	{"F1"             , "37", KEY_F1},
	{"TV/SAT"         , "DD", KEY_AUX},
	{"KEY_1"          , "3F", KEY_1},
	{"KEY_2"          , "07", KEY_2},
	{"KEY_3"          , "BD", KEY_3},
	{"KEY_4"          , "5D", KEY_4},
	{"KEY_5"          , "1F", KEY_5},
	{"KEY_6"          , "9D", KEY_6},
	{"KEY_7"          , "55", KEY_7},
	{"KEY_8"          , "17", KEY_8},
	{"KEY_9"          , "95", KEY_9},
	{"KEY_0"          , "27", KEY_0},
	{"TV/RADIO"       , "65", KEY_TV2}, //WE USE TV2 AS TV/RADIO SWITCHB
	{"RECALL"         , "A5", KEY_BACK},
	{"FIND"           , "75", KEY_FIND},
	{"REC"            , "D5", KEY_RECORD},
	{"SAT"            , "A7", KEY_SAT},
	{"FAV"            , "B5", KEY_FAVORITES},
	{"MENU"           , "1D", KEY_MENU},
	{"INFO"           , "5F", KEY_INFO},
	{"OK"             , "BF", KEY_OK},
	{"UP"             , "9F", KEY_UP},
	{"DOWN"           , "AF", KEY_DOWN},
	{"LEFT"           , "3D", KEY_LEFT},
	{"RIGHT"          , "7F", KEY_RIGHT},
	{"EXIT"           , "2D", KEY_HOME},
	{"EPG"            , "6F", KEY_EPG},
	{"FOLDER"         , "0D", KEY_ARCHIVE},
	{"STOP"           , "8F", KEY_STOP},
	{"PAUSE"          , "CD", KEY_PAUSE},
	{"PLAY"           , "4F", KEY_PLAY},
	{"PREV"           , "35", KEY_PREVIOUS},
	{"NEXT"           , "B7", KEY_NEXT},
	{"FASTFORWARD"    , "77", KEY_FASTFORWARD},
	{"REWIND"         , "F5", KEY_REWIND},
	{"FAST"           , "97", KEY_F},
	{"SLOW"           , "15", KEY_SLOW},
	{"PLAY_MODE"      , "E5", KEY_P},
	{"USB"            , "67", KEY_CLOSE},
	{"UHF"            , "0F", KEY_U},
	{"AUDIO"          , "25", KEY_SUBTITLE},
	{"RED"            , "05", KEY_RED},
	{"GREEN"          , "87", KEY_GREEN},
	{"YELLOW"         , "C5", KEY_YELLOW},
	{"BLUE"           , "47", KEY_BLUE},
	{""               , ""  , KEY_NULL}
};

/* GALAXY RC */
static tButton cButtonsGalaxy[] =
{
	{"POWER"          , "25", KEY_POWER},
	{"R"              , "A5", KEY_R},
	{"V.FORMAT"       , "AD", KEY_V},
	{"TIME"           , "8D", KEY_TIME},
	{"MUTE"           , "85", KEY_MUTE},
	{"TV/SAT"         , "C5", KEY_AUX},
	{"Tms"            , "E5", KEY_T},
	{"PRESENTATION"   , "ED", KEY_PRESENTATION},
	{"F1"             , "CD", KEY_F1},
	{"0BUTTON"        , "57", KEY_0},
	{"1BUTTON"        , "B5", KEY_1},
	{"2BUTTON"        , "95", KEY_2},
	{"3BUTTON"        , "BD", KEY_3},
	{"4BUTTON"        , "F5", KEY_4},
	{"5BUTTON"        , "D5", KEY_5},
	{"6BUTTON"        , "FD", KEY_6},
	{"7BUTTON"        , "35", KEY_7},
	{"8BUTTON"        , "15", KEY_8},
	{"9BUTTON"        , "3D", KEY_9},
	{"TV/RADIO"       , "77", KEY_TV2},
	{"RECALL"         , "7F", KEY_BACK},
	{"VOL+"           , "C7", KEY_VOLUMEDOWN},
	{"VOL-"           , "DD", KEY_VOLUMEUP},
	{"PAGE-"          , "5F", KEY_PAGEDOWN},
	{"PAGE+"          , "07", KEY_PAGEUP},
	{"FIND"           , "9D", KEY_FIND},
	{"SAT"            , "1D", KEY_SAT},
	{"REC"            , "45", KEY_RECORD},
	{"FAV"            , "87", KEY_FAVORITES},
	{"MENU"           , "65", KEY_MENU},
	{"INFO"           , "A7", KEY_INFO},
	{"EXIT"           , "4D", KEY_EXIT},
	{"EPG"            , "8F", KEY_EPG},
	{"OK"             , "2F", KEY_OK},
	{"UP"             , "27", KEY_UP},
	{"DOWN"           , "0F", KEY_DOWN},
	{"LEFT"           , "6D", KEY_LEFT},
	{"RIGHT"          , "AF", KEY_RIGHT},
	{"FOLDER"         , "75", KEY_ARCHIVE},
	{"STOP"           , "F7", KEY_STOP},
	{"PAUSE"          , "37", KEY_PAUSE},
	{"PLAY"           , "B7", KEY_PLAY},
	{"PREV"           , "55", KEY_PREVIOUS},
	{"NEXT"           , "D7", KEY_NEXT},
	{"REWIND"         , "17", KEY_REWIND},
	{"FORWARD"        , "97", KEY_FORWARD},
	{"USB"            , "9F", KEY_CLOSE},
	{"RED"            , "7D", KEY_RED},
	{"GREEN"          , "FF", KEY_GREEN},
	{"YELLOW"         , "3F", KEY_YELLOW},
	{"BLUE"           , "BF", KEY_BLUE},
	{"PLAY_MODE"      , "1F", KEY_P},
	{"SLOW"           , "5D", KEY_SLOW},
	{"FAST"           , "DF", KEY_FASTFORWARD},
	{""               , ""  , KEY_NULL}
};

/* spark Default */
static tButton cButtonsSparkEdv[] =
{
	{"POWER"          , "87", KEY_POWER},
	{"V.FORMAT"       , "2f", KEY_V},
	{"TV/SAT"         , "95", KEY_AUX},
	{"TIME"           , "65", KEY_TIME},
	{"MUTE"           , "A5", KEY_MUTE},
	{"0BUTTON"        , "37", KEY_0},
	{"1BUTTON"        , "A7", KEY_1},
	{"2BUTTON"        , "07", KEY_2},
	{"3BUTTON"        , "E5", KEY_3},
	{"4BUTTON"        , "97", KEY_4},
	{"5BUTTON"        , "27", KEY_5},
	{"6BUTTON"        , "D5", KEY_6},
	{"7BUTTON"        , "B7", KEY_7},
	{"8BUTTON"        , "17", KEY_8},
	{"9BUTTON"        , "F5", KEY_9},
	{"TV/RADIO"       , "CD", KEY_TV2}, //WE USE TV2 AS TV/RADIO SWITCHB
	{"RECALL"         , "CF", KEY_BACK},
	{"PAGE+"          , "ED", KEY_PAGEUP},
	{"PAGE-"          , "DD", KEY_PAGEDOWN},
	{"FIND"           , "C5", KEY_FIND},
	{"FOLDER"         , "67", KEY_ARCHIVE},
	{"VOL+"           , "AF", KEY_VOLUMEUP},
	{"VOL-"           , "9F", KEY_VOLUMEDOWN},
	{"MENU"           , "C7", KEY_MENU},
	{"INFO"           , "1F", KEY_INFO},
	{"UP"             , "3F", KEY_UP},
	{"DOWN"           , "85", KEY_DOWN},
	{"LEFT"           , "BF", KEY_LEFT},
	{"RIGHT"          , "FD", KEY_RIGHT},
	{"OK"             , "05", KEY_OK},
	{"EXIT"           , "F9", KEY_HOME},
	{"EPG"            , "45", KEY_EPG},
	{"FAV"            , "3D", KEY_FAVORITES},
	{"SAT"            , "0D", KEY_SAT},
	{"RED"            , "6D", KEY_RED},
	{"GREEN"          , "8D", KEY_GREEN},
	{"YELLOW"         , "77", KEY_YELLOW},
	{"BLUE"           , "AD", KEY_BLUE},
	{"REC"            , "F7", KEY_RECORD},
	{"STOP"           , "BB", KEY_STOP},
	{"PLAY"           , "57", KEY_PLAY},
	{"PAUSE"          , "4D", KEY_PAUSE},
	{"FASTFORWARD"    , "35", KEY_FASTFORWARD},
	{"REWIND"         , "7F", KEY_REWIND},
	{"PREV"           , "FB", KEY_PREVIOUS},
	{"NEXT"           , "3B", KEY_NEXT},
	{"FAST"           , "E7", KEY_F},
	{"SLOW"           , "7B", KEY_SLOW},
	{"PLAY_MODE"      , "B5", KEY_P},
	{"USB"            , "DF", KEY_CLOSE},
	{"Tms"            , "55", KEY_T},
	{"F1"             , "15", KEY_F1},
	{"F2"             , "D1", KEY_F2},
	{""               , ""  , KEY_NULL}
};

/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */

void Get_StbId()
{
	char *pch;
	int fn = open("/proc/cmdline", O_RDONLY);

	if (fn > -1)
	{
		char procCmdLine[1024];
		int len = read(fn, procCmdLine, sizeof(procCmdLine) - 1);
		if (len > 0)
		{
			procCmdLine[len] = 0;

			pch = strstr(procCmdLine, "STB_ID=");
			strncpy(VendorStbId, pch + 7, 8);
			printf("Vendor STB-ID=%s\n", VendorStbId);
		}
		close(fn);
	}
}

static struct sockaddr_un  vAddr;

static tButton *pSparkGetButton(char *pData)

{
	tButton	*pButtons = cButtonsEdisionSpark;

	if (!strncasecmp(pData, SPARK_RC05_PREDATA, sizeof(SPARK_RC05_PREDATA)))
	{
		pButtons = cButtonsEdisionSpark;
	}
	else if (!strncasecmp(pData, SPARK_RC08_PREDATA, sizeof(SPARK_RC08_PREDATA)))
	{
		pButtons = cButtonsSparkRc08;
	}
	else if (!strncasecmp(pData, SPARK_RC09_PREDATA, sizeof(SPARK_RC09_PREDATA)))
	{
		static tButton *cButtons = NULL;
		if (!cButtons)
		{
			if (strstr(STB_ID_EDISION_PINGULUX, VendorStbId))
				cButtons = cButtonsEdisionSpark;
			else if (strstr(STB_ID_GALAXYINNOVATIONS_S8120, VendorStbId))
				cButtons = cButtonsGalaxy;
			else
				cButtons = cButtonsSparkRc09; /* Amiko Alien 8900 */
		}
		return cButtons;
#if 0
		if (!cButtons)
		{
			int fn = open("/proc/cmdline", O_RDONLY);

			if (fn > -1)
			{
				char procCmdLine[1024];
				int len = read(fn, procCmdLine, sizeof(procCmdLine) - 1);

				if (len > 0)
				{
					procCmdLine[len] = 0;

					if (strstr(procCmdLine, "STB_ID=" STB_ID_EDISION_PINGULUX))
						cButtons = cButtonsEdisionSpark;

					if (strstr(procCmdLine, "STB_ID=" STB_ID_GALAXYINNOVATIONS_S8120))
						cButtons = cButtonsGalaxy;
				}

				close(fn);
			}

			if (!cButtons)
				cButtons = cButtonsSparkRc09; /* Amiko Alien 8900 */
		}

		return cButtons;
#endif
	}
	else if (!strncasecmp(pData, SPARK_DEFAULT_PREDATA, sizeof(SPARK_DEFAULT_PREDATA)))
	{
		static tButton *cButtons = NULL;
		if (!cButtons)
		{
			if (strstr(STB_ID_SOGNO_TRIPLE_HD, VendorStbId))
				cButtons = cButtonsSognoTriplex;
			else if (strstr(STB_ID_SAB_UNIX_TRIPLE_HD, VendorStbId))
				cButtons = cButtonsSparkDefault;
			else
				cButtons = cButtonsSparkDefault;
		}
		return cButtons;
	}
	else if (!strncasecmp(pData, UFS910_RC660_PREDATA, sizeof(UFS910_RC660_PREDATA)))
	{
		pButtons = cButtonsUfs910Rc660;
	}
	else if (!strncasecmp(pData, UFS913_RC230_PREDATA, sizeof(UFS913_RC230_PREDATA)))
	{
		pButtons = cButtonsUfs913Rc230;
	}
	else if (!strncasecmp(pData, SAMSUNG_AA59_PREDATA, sizeof(SAMSUNG_AA59_PREDATA)))
	{
		pButtons = cButtonsSamsungAA59;
	}
	else if (!strncasecmp(pData, SPARK_RC12_PREDATA, sizeof(SPARK_RC12_PREDATA)))
	{
		pButtons = cButtonsSparkRc12;
	}
	else if (!strncasecmp(pData, SPARK_RC04_PREDATA, sizeof(SPARK_RC04_PREDATA)))
	{
		pButtons = cButtonsSparkRc04;
	}
	else if (!strncasecmp(pData, SPARK_EDV_RC1, sizeof(SPARK_EDV_RC1)))
	{
		pButtons = cButtonsSparkEdv;
	}
	else if (!strncasecmp(pData, SPARK_EDV_RC2, sizeof(SPARK_EDV_RC2)))
	{
		pButtons = cButtonsSparkEdv;
	}

	return pButtons;
}

static int pInit(Context_t *context, int argc, char *argv[])
{
	int vHandle;

	Get_StbId();

	vAddr.sun_family = AF_UNIX;
	strcpy(vAddr.sun_path, "/var/run/lirc/lircd");

	vHandle = socket(AF_UNIX, SOCK_STREAM, 0);

	if (vHandle == -1)
	{
		perror("socket");
		return -1;
	}

	if (connect(vHandle, (struct sockaddr *)&vAddr, sizeof(vAddr)) == -1)
	{
		perror("connect");
		return -1;
	}

	return vHandle;
}

static int pShutdown(Context_t *context)
{
	close(context->fd);

	return 0;
}

static int pRead(Context_t *context)
{
	char                vBuffer[128];
	char                vData[10];
	const int           cSize         = 128;
	int                 vCurrentCode  = -1;
	int                 rc;
	tButton             *cButtons = cButtonsEdisionSpark;

	memset(vBuffer, 0, 128);
	//wait for new command
	rc = read(context->fd, vBuffer, cSize);

	if (rc <= 0)return -1;

	//parse and send key event
	vData[0] = vBuffer[17];
	vData[1] = vBuffer[18];
	vData[2] = '\0';


	vData[0] = vBuffer[8];
	vData[1] = vBuffer[9];
	vData[2] = vBuffer[10];
	vData[3] = vBuffer[11];
	vData[4] = '\0';
	cButtons = pSparkGetButton(vData);

	vData[0] = vBuffer[14];
	vData[1] = vBuffer[15];
	vData[2] = '\0';

	printf("[RCU] key: %s -> %s\n", vData, &vBuffer[0]);
	vCurrentCode = getInternalCode(cButtons, vData);

	if (vCurrentCode != 0)
	{
		static int nextflag = 0;

		if (('0' == vBuffer[17]) && ('0' == vBuffer[18]))
		{
			nextflag++;
		}

		vCurrentCode += (nextflag << 16);
	}

	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	struct aotom_ioctl_data vfd_data;
	int ioctl_fd = -1;

	if (cOn)
	{
		ioctl_fd = open("/dev/vfd", O_RDONLY);
		vfd_data.u.icon.icon_nr = 35;
		vfd_data.u.icon.on = 1;
		ioctl(ioctl_fd, VFDICONDISPLAYONOFF, &vfd_data);
		close(ioctl_fd);
	}
	else
	{
		usleep(100000);
		ioctl_fd = open("/dev/vfd", O_RDONLY);
		vfd_data.u.icon.icon_nr = 35;
		vfd_data.u.icon.on = 0;
		ioctl(ioctl_fd, VFDICONDISPLAYONOFF, &vfd_data);
		close(ioctl_fd);
	}

	return 0;
}

RemoteControl_t Spark_RC =
{
	"Spark RemoteControl",
	Spark,
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification,
	cButtonsEdisionSpark,
	NULL,
	NULL,
	1,
	&cLongKeyPressSupport,
};

