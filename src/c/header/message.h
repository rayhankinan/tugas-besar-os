#ifndef __MESSAGE__H
#define __MESSAGE__H

#include "std_type.h"

extern int interrupt(int int_number, int AX, int BX, int CX, int DX);
struct message
{
  byte current_directory;
  char arg1[64];
  char arg2[64];
  char arg3[64];
  int next_program_segment; // Dapat digunakan untuk bagian 3.5

  byte other[315];
};


void getMessage(struct message *m, int segment);
void sendMessage(struct message *m, int segment);

#endif