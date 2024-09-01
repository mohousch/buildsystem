#include <types.h>
#include "mxl_common.h"

/* ========================== Local functions called by Init and RFTune ============================ */
UINT32 SetIRVBit(PIRVType pIRV, UINT8 Num, UINT8 Mask, UINT8 Val)
/* Updated to have 0xFF as the limit - hchan 18/02/2010 */
{
	/* while (pIRV->Num || pIRV->Val ) */
	/* while (pIRV->Num != 0xFF && pIRV->Val != 0xFF)*/
	while (pIRV->Num != 0xFF || pIRV->Val != 0xFF)
	{
		if (pIRV->Num == Num)
		{
			pIRV->Val &= ~Mask;
			pIRV->Val |= Val;
		}
		pIRV++;
	}
	return MxL_OK;
}

UINT32 SetIRVBit_Ext(PIRVType pIRV, UINT8 Num1, UINT8 Mask, UINT8 Val1, UINT8 Val2)
/* Updated to have 0xFF as the limit - hchan 18/02/2010 */
{
	while (pIRV->Num != 0xFF || pIRV->Val != 0xFF)
	{
		if (pIRV->Num == Num1 && pIRV->Val == Val1)
		{
			(pIRV + 1)->Val &= ~Mask;
			(pIRV + 1)->Val |= Val2;
			return MxL_OK;
		}
		pIRV++;
	}
	return MxL_OK;
}

/* Have to Implement a local version of the round function -
 Removed to support integers only : hchan 27/02/2009 */
/*SINT32 round_d(double inVal)
{
 double modulus = 0.0;
 int intVal = (int)(inVal);
 int retVal = 0;
 modulus = inVal - intVal;

 if (modulus >= 0.5 && inVal >0)
 retVal = (intVal + 1);
 else if(modulus < 0.5 && inVal > 0)
 retVal = (intVal);
 else if(-(modulus) >= 0.5 && inVal < 0)
 retVal = (intVal - 1);
 else if(-(modulus) < 0.5 && inVal < 0)
 retVal = (intVal);
 else retVal = (0);

 return(retVal);
}
*/

UINT32 div_rnd_uint32(UINT32 numerator, UINT32 denominator)
{
	UINT32 quotient = numerator / denominator;
	UINT32 remainder = numerator % denominator;
	/* round up based on teh remainder and denominator */
	if (denominator % 2 == 1) /*an odd denominator */
	{
		if (remainder >= denominator / 2)
			quotient++;
	}
	else /* and even denominator */
	{
		if (remainder > denominator / 2)
			quotient ++;
	}
	return (quotient);
}
