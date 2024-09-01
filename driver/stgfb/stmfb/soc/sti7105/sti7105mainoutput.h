/***********************************************************************
 *
 * File: soc/sti7105/sti7105mainoutput.h
 * Copyright (c) 2010 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#ifndef _STi7105MAINOUTPUT_H
#define _STi7105MAINOUTPUT_H

#include <soc/sti7111/sti7111mainoutput.h>
#include <HDTVOutFormatter/stmtvoutreg.h>
#include "sti7105reg.h"

class CSTi7105MainOutput: public CSTi7111MainOutput
{
public:
  CSTi7105MainOutput(CDisplayDevice *pDev,
                     CSTmSDVTG *pVTG1,
                     CSTmSDVTG *pVTG2,
                     CSTmTVOutDENC *pDENC,
                     CDisplayMixer *pMixer,
                     CSTmFSynth *pFSynth1,
                     CSTmFSynth *pFSynth2,
                     CSTmHDFormatterAWG *pAWG,
                     stm_plane_id_t sharedVideoPlaneID,
                     stm_plane_id_t sharedGDPPlaneID): CSTi7111MainOutput(pDev,pVTG1,pVTG2,pDENC,
                                                                          pMixer,pFSynth1,pFSynth2,pAWG,
                                                                          sharedVideoPlaneID,sharedGDPPlaneID)
  {
    /*
     * Additional glue so that when sysconfig6[6] is set to select DVO0 syncs
     * from "main" it gets the same sync set as selected for the PADs in the
     * main TVOUT pads control register.
     */
    DENTRY();
    ULONG tmp = ReadAuxTVOutReg(TVOUT_GP_OUT) & ~(TVOUT_GP_HSYNC_MUX_MAIN(TVOUT_GP_HSYNC_MAIN_MASK) |
                                                  TVOUT_GP_VSYNC_MUX_MAIN(TVOUT_GP_VSYNC_MASK));

    tmp |= (TVOUT_GP_HSYNC_MUX_MAIN(TVOUT_GP_HSYNC_MAIN_2) |
            TVOUT_GP_VSYNC_MUX_MAIN(TVOUT_GP_VSYNC_2));

    WriteAuxTVOutReg(TVOUT_GP_OUT, tmp);
    DEXIT();
  }

private:
  CSTi7105MainOutput(const CSTi7105MainOutput&);
  CSTi7105MainOutput& operator=(const CSTi7105MainOutput&);
};


#endif //_STi7105MAINOUTPUT_H
