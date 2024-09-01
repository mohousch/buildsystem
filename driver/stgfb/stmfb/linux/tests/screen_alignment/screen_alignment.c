#include <directfb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define DFBCHECK(x...)					\
{							\
	err = x;					\
	if (err != DFB_OK) {					  \
		fprintf(stderr,"%s <%d>:\n\t",__FILE__,__LINE__); \
		DirectFBErrorFatal( #x, err);			  \
	}							  \
}

int main(int argc, char *argv[])
{
DFBResult err;

IDirectFB             *dfb;
IDirectFBSurface      *primary;
DFBSurfaceDescription dsc;
int width, height;

  DFBCHECK(DirectFBInit( &argc, &argv ));

  DFBCHECK(DirectFBCreate( &dfb ));

  DFBCHECK(dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN ));
  dsc.flags = DSDESC_CAPS;
  dsc.caps = DSCAPS_PRIMARY;
  DFBCHECK(dfb->CreateSurface( dfb, &dsc, &primary ));

  primary->GetSize(primary, &width, &height);
  primary->SetColor(primary, 255, 0, 0, 255);
  primary->FillRectangle(primary, 0, 0, width, height);
  primary->SetColor(primary, 255, 255, 255, 255);
  primary->DrawRectangle(primary, 0, 0, width, height);
  primary->DrawLine(primary,0,0,width-1,height-1);
  primary->DrawLine(primary,0,height-1,width-1,0);
  primary->DrawLine(primary,0,height/2,width-1,height/2);
  primary->DrawLine(primary,width/2,0,width/2,height-1);

  getchar();

  return 0;
}
