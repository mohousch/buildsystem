/***********************************************************************
 *
 * File: soc/sti7105/sti7105auxoutput.h
 * Copyright (c) 2010 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STi7105AUXOUTPUT_H
#define _STi7105AUXOUTPUT_H

#include <soc/sti7111/sti7111auxoutput.h>
#include <HDTVOutFormatter/stmtvoutreg.h>
#include "sti7105reg.h"

class CSTi7105Device;

class CSTi7105AuxOutput: public CSTi7111AuxOutput
{
public:
  CSTi7105AuxOutput(CSTi7105Device        *pDev,
                    CSTmSDVTG             *pVTG,
                    CSTmTVOutDENC         *pDENC,
                    CDisplayMixer         *pMixer,
                    CSTmFSynth            *pFSynth,
                    CSTmHDFormatterAWG    *pAWG,
                    CSTmHDFormatterOutput *pHDFOutput,
                    stm_plane_id_t         sharedVideoPlaneID,
                    stm_plane_id_t         sharedGDPPlaneID): CSTi7111AuxOutput(pDev,pVTG,pDENC,pMixer,
                                                                                pFSynth,pAWG,pHDFOutput,
                                                                                sharedVideoPlaneID,
                                                                                sharedGDPPlaneID)
  {
    /*
     * Additional glue to allow the correct aux sync set to get to the pads if
     * bit 0 of the register is set and the pads are set to output "main"
     * rather than vtg0. It is not clear there is a real use for this (as
     * opposed to chip validation) but let us be nice and set it up just in case
     */
    DENTRY();
    ULONG tmp = ReadAuxTVOutReg(TVOUT_GP_OUT) & ~(TVOUT_GP_HSYNC_MUX_AUX(TVOUT_GP_HSYNC_AUX_MASK) |
                                                  TVOUT_GP_VSYNC_MUX_AUX(TVOUT_GP_VSYNC_MASK));

    tmp |= (TVOUT_GP_HSYNC_MUX_AUX(TVOUT_GP_HSYNC_AUX_2) |
            TVOUT_GP_VSYNC_MUX_AUX(TVOUT_GP_VSYNC_2));

    WriteAuxTVOutReg(TVOUT_GP_OUT, tmp);
    DEXIT();
  }

private:
  CSTi7105AuxOutput(const CSTi7105AuxOutput&);
  CSTi7105AuxOutput& operator=(const CSTi7105AuxOutput&);
};


#endif //_STi7105AUXOUTPUT_H
