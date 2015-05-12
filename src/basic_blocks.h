#ifndef _BASIC_BLOCKS_H
#define _BASIC_BLOCKS_H

#include <stdio.h>

struct ir_instruction;
struct ir_section;

struct basic_block {
  struct ir_section * ir_section;
  struct ir_instruction *beginning;
  struct ir_instruction *end;
  struct basic_block *next;
  struct basic_block *left, *right;
};

void remove_no_ops_from_ir(struct ir_section **root_ir);

void remove_redundant_gotos(struct ir_section **root_ir);

void remove_redundant_labels(struct ir_section **root_ir);

struct basic_block * get_basic_blocks_from_ir(struct ir_section *root_ir);

void propagate_constant_values(struct ir_section **root_ir);
#endif /* _BASIC_BLOCKS_H */
