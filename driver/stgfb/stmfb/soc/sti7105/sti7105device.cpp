/***********************************************************************
 *
 * File: soc/sti7105/sti7105device.cpp
 * Copyright (c) 2008,2009 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <stmdisplay.h>

#include <Generic/IOS.h>
#include <Generic/IDebug.h>

#include <Gamma/GDPBDispOutput.h>

#include <STMCommon/stmsdvtg.h>
#include <STMCommon/stmfsynth.h>
#include <HDTVOutFormatter/stmhdfawg.h>
#include <HDTVOutFormatter/stmtvoutdenc.h>

#include <soc/sti7111/sti7111hdmi.h>
#include <soc/sti7111/sti7111dvo.h>

#include "sti7105reg.h"
#include "sti7105device.h"
#include "sti7105mainoutput.h"
#include "sti7105auxoutput.h"


CSTi7105Device::CSTi7105Device (void): CSTi7111Device ()
{
  DEBUGF2 (2, (FENTRY " @ %p\n", __PRETTY_FUNCTION__, this));

  DEBUGF2 (2, (FEXIT " @ %p\n", __PRETTY_FUNCTION__, this));
}


CSTi7105Device::~CSTi7105Device (void)
{
  DEBUGF2 (2, (FENTRY " @ %p\n", __PRETTY_FUNCTION__, this));

  DEBUGF2 (2, (FEXIT " @ %p\n", __PRETTY_FUNCTION__, this));
}



bool CSTi7105Device::CreateOutputs(void)
{
  DENTRY();

  CSTi7105MainOutput *pMainOutput;
  if((pMainOutput = new CSTi7105MainOutput(this, m_pVTG1,m_pVTG2, m_pDENC, m_pMainMixer, m_pFSynthHD, m_pFSynthSD, m_pAWGAnalog, m_sharedVideoPlaneID, m_sharedGDPPlaneID)) == 0)
  {
    DERROR("failed to create main output\n");
    return false;
  }
  m_pOutputs[STi7111_OUTPUT_IDX_VDP0_MAIN] = pMainOutput;

  if((m_pOutputs[STi7111_OUTPUT_IDX_VDP1_MAIN] = new CSTi7105AuxOutput(this, m_pVTG2, m_pDENC, m_pAuxMixer, m_pFSynthSD, m_pAWGAnalog, pMainOutput, m_sharedVideoPlaneID, m_sharedGDPPlaneID)) == 0)
  {
    DERROR("failed to create aux output\n");
    return false;
  }

  if(!this->CreateHDMIOutput())
    return false;

  CSTi7111DVO *pFDVO = new CSTi7111DVO (this, pMainOutput,
                                              STi7111_FLEXDVO_BASE,
                                              STi7111_HD_FORMATTER_BASE);
  if(!pFDVO)
  {
    DERROR("failed to create FDVO output\n");
    return false;
  }
  m_pOutputs[STi7111_OUTPUT_IDX_DVO0] = pFDVO;

  pMainOutput->SetSlaveOutputs(m_pOutputs[STi7111_OUTPUT_IDX_DVO0],
                               m_pOutputs[STi7111_OUTPUT_IDX_VDP0_HDMI]);

  m_pGDPBDispOutput = new CGDPBDispOutput(this, pMainOutput,
                                          m_pGammaReg + (STi7111_BLITTER_BASE>>2),
                                          STi7111_REGISTER_BASE+STi7111_BLITTER_BASE,
                                          BDISP_CQ1_BASE,1,2);

  if(m_pGDPBDispOutput == 0)
  {
    DERROR("failed to allocate BDisp virtual plane output");
    return false;
  }

  if(!m_pGDPBDispOutput->Create())
  {
    DERROR("failed to create BDisp virtual plane output");
    return false;
  }

  m_pOutputs[STi7111_OUTPUT_IDX_GDP] = m_pGDPBDispOutput;
  m_nOutputs = 5;

  /*
   * TODO: Create DVO1 output
   */

  DEXIT();
  return true;
}
