#ifndef _MRA_H_
#define _MRA_H_

#include "sxmlc.h"
#include "globals.h"

void mra_read_rom(XMLNode *node, t_rom *rom);
void mra_read_files(XMLNode *node, t_files *files);

#endif