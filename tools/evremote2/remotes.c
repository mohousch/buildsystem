/*
 * evremote.c
 *
 * (c) 2009 donald@teamducktales
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

#include "remotes.h"

static RemoteControl_t *AvailableRemoteControls[] =
{
	&Ufs910_1W_RC,
	&Ufs910_14W_RC,
	&Tf7700_RC,
	&Hl101_RC,
	&Vip2_RC,
	&UFS922_RC,
	&UFC960_RC,
	&Fortis_RC,
	&Hs5101_RC,
	&UFS912_RC,
	&Spark_RC,
	&Adb_Box_RC,
	&Cuberevo_RC,
	&Ipbox_RC,
	&CNBOX_RC,
	&VitaminHD5000_RC,
	&LircdName_RC,
	NULL
};

int selectRemote(Context_t  *context, eBoxType type)
{
	int i;

	for (i = 0; AvailableRemoteControls[i] != 0ull; i++)

		if (AvailableRemoteControls[i]->Type == type)
		{
			context->r = AvailableRemoteControls[i];
			return 0;
		}

	return -1;
}
