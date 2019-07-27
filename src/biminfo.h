#ifndef _BIM_BIMINFO_H
#define _BIM_BIMINFO_H

#include <stdio.h>
#include "buffer.h"

extern FILE * open_biminfo(void);
extern int fetch_from_biminfo(buffer_t * buf);
extern int update_biminfo(buffer_t * buf);

#endif // _BIM_BIMINFO_H
