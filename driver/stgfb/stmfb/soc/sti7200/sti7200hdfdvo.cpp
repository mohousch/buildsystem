/***********************************************************************
 *
 * File: soc/sti7200/sti7200hdfdvo.cpp
 * Copyright (c) 2008-2009 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
\***********************************************************************/

#include <stmdisplay.h>

#include <Generic/IOS.h>
#include <Generic/IDebug.h>

#include <Generic/DisplayDevice.h>

#include "sti7200hdfdvo.h"
#include "sti7200reg.h"

/*
 * This is OK, even for Cut2 given the limited registers used.
 */
#include "sti7200cut1tvoutreg.h"

#define TVOUT_CLK_SEL_PIX_MAIN_SHIFT   (0)
#define TVOUT_CLK_SEL_PIX_MAIN_MASK    (3L<<TVOUT_CLK_SEL_PIX_MAIN_SHIFT)
#define TVOUT_CLK_SEL_FDVO_MAIN_SHIFT  (6)
#define TVOUT_CLK_SEL_FDVO_MAIN_MASK   (3L<<TVOUT_CLK_SEL_FDVO_MAIN_SHIFT)

CSTi7200HDFDVO::CSTi7200HDFDVO(CDisplayDevice *pDev,
                               COutput        *pMasterOutput,
                               ULONG           ulLocalDisplayBase,
                               ULONG           ulHDFormatterBase): CSTmHDFDVO(pDev,ulLocalDisplayBase+STi7200_FDVO_OFFSET,ulHDFormatterBase,pMasterOutput)
{
  DEBUGF2(2,("CSTi7200HDFDVO::CSTi7200HDFDVO pDev = %p  master output = %p\n",pDev,pMasterOutput));
  m_pTVOutRegs = (ULONG*)pDev->GetCtrlRegisterBase() + ((ulLocalDisplayBase+STi7200_MAIN_TVOUT_OFFSET)>>2);
  m_pSysCfgRegs = (ULONG*)pDev->GetCtrlRegisterBase() + (STi7200_SYSCFG_BASE>>2);

  ULONG val = ReadSysCfgReg(SYS_CFG40) & ~(SYS_CFG40_DVO_AUX_NOT_MAIN |
                                           SYS_CFG40_DVO_TO_DVP_EN    |
                                           SYS_CFG40_DVO_OUT_PIO_EN);
  WriteSysCfgReg(SYS_CFG40,val);

  m_bOutputToDVP = false;
  DEXIT();
}


CSTi7200HDFDVO::~CSTi7200HDFDVO() {}


bool CSTi7200HDFDVO::Stop(void)
{
  DENTRY();

  /*
   * Disable DVO on PIO pins
   */
  ULONG val = ReadSysCfgReg(SYS_CFG40) & ~SYS_CFG40_DVO_OUT_PIO_EN;
  WriteSysCfgReg(SYS_CFG40,val);

  CSTmFDVO::Stop();

  DEXIT();
  return true;
}


bool CSTi7200HDFDVO::SetOutputFormat(ULONG format,const stm_mode_line_t* pModeLine)
{
  DENTRY();

  if(!CSTmHDFDVO::SetOutputFormat(format,pModeLine))
    return false;

  if(pModeLine)
  {
    ULONG clkconfig = ReadTVOutReg(TVOUT_CLK_SEL) & ~TVOUT_CLK_SEL_FDVO_MAIN_MASK;
    ULONG pixclkdiv = (clkconfig & TVOUT_CLK_SEL_PIX_MAIN_MASK);

    switch(format)
    {
      case STM_VIDEO_OUT_RGB:
      case STM_VIDEO_OUT_YUV_24BIT:
      case STM_VIDEO_OUT_422:
      {
        /*
         * DVO clock same as pixel clock
         */
        DEBUGF2(2,("%s: FDVO clock == PIX clock\n",__PRETTY_FUNCTION__));
        clkconfig |= pixclkdiv << TVOUT_CLK_SEL_FDVO_MAIN_SHIFT;
        break;
      }
      case STM_VIDEO_OUT_YUV:
      case STM_VIDEO_OUT_ITUR656:
      {
        /*
         * DVO clock 2x pixel clock, not possible if pixel clock == reference
         * clock (looking forward to 1080p on Cut2)
         */
        if(pixclkdiv == TVOUT_CLK_BYPASS)
        {
          DEBUGF2(1,("%s: Cannot set 2xPIX clock\n",__PRETTY_FUNCTION__));
          return false;
        }

        DEBUGF2(2,("%s: FDVO clock == 2xPIX clock\n",__PRETTY_FUNCTION__));
        clkconfig |= (pixclkdiv-1) << TVOUT_CLK_SEL_FDVO_MAIN_SHIFT;
        break;
      }
      default:
        DEBUGF2(2,("%s: Invalid format?\n",__PRETTY_FUNCTION__));
        /*
         * Disable DVO on PIO pins
         */
        ULONG val = ReadSysCfgReg(SYS_CFG40) & ~SYS_CFG40_DVO_OUT_PIO_EN;
        WriteSysCfgReg(SYS_CFG40,val);
        return false;
    }

    WriteTVOutReg(TVOUT_CLK_SEL,clkconfig);
    /*
     * Enable DVO on PIO pins
     */
    ULONG val = ReadSysCfgReg(SYS_CFG40) | SYS_CFG40_DVO_OUT_PIO_EN;
    WriteSysCfgReg(SYS_CFG40,val);
  }

  return true;
}

