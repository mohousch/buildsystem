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
extern RemoteControl_t LircdName_RC;

static RemoteControl_t *AvailableRemoteControls[] =
{
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
