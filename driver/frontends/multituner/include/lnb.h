#ifndef lnb_123
#define lnb_123

#define cLNB_LNBH221 1
#define cLNB_PIO     2
#define cLNB_A8293   3

#include "equipment.h"
#include "frontend_platform.h"  // for dprintk definition

extern void *lnbh221_attach(u32 *lnb, struct equipment_s *equipment);
extern void *lnb_pio_attach(u32 *lnb, struct equipment_s *equipment);
extern void *lnb_a8293_attach(u32 *lnb, struct equipment_s *equipment);
#endif
// vim:ts=4
