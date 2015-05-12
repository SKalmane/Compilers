#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "node.h"
#include "symbol.h"
#include "type.h"
#include "basic_blocks.h"
#include "ir.h"

void ir_remove_next_instruction(struct ir_instruction *instruction) {
  assert(instruction->next != NULL);

  instruction->next = instruction->next->next;
  instruction->next->prev = instruction;
}

void remove_no_ops_from_ir(struct ir_section **root_ir) {
  struct ir_section *ir_section = *root_ir;
  struct ir_instruction *instruction = ir_section->first;

  while(instruction->next != NULL) {
    if(instruction->next->kind == IR_NO_OPERATION) {
      ir_remove_next_instruction(instruction);
    }
    if(instruction->next->kind != IR_NO_OPERATION) {
      instruction = instruction->next;
    }
  }

  /* Cleaning up the remaining no-ops */
  if(ir_section->first->kind == IR_NO_OPERATION) {
    ir_section->first = ir_section->first->next;
    ir_section->first->next->prev = NULL;
  }

  *root_ir = ir_section;
}

void remove_redundant_gotos(struct ir_section **root_ir) {
  struct ir_section *ir_section = *root_ir;
  struct ir_instruction *instruction = ir_section->first;
  int generated_label;
  struct ir_instruction *temp_instruction;

  while(instruction->next != NULL) {
    if(instruction->kind == IR_GOTO) {
      assert(instruction->operands[0].kind == OPERAND_GENERATED_LABEL);
      generated_label = instruction->operands[0].data.generated_label;
      temp_instruction = instruction->next;
      while((temp_instruction != NULL) &&
	    (temp_instruction->kind == IR_GENERATED_LABEL)) {
	if(temp_instruction->operands[0].data.generated_label == generated_label) {
	  ir_remove_next_instruction(instruction->prev);
	  break;
	} else {
	  temp_instruction = temp_instruction->next;
	}
      }

      temp_instruction = instruction->next;
      while((temp_instruction != NULL) &&
	    (temp_instruction->kind == IR_GOTO)) {
	ir_remove_next_instruction(temp_instruction->prev);
	temp_instruction = temp_instruction->next;
      }
    }
    instruction = instruction->next;
  }

  *root_ir = ir_section;
}

void remove_redundant_labels(struct ir_section **root_ir) {
  struct ir_section *ir_section = *root_ir;
  struct ir_instruction *instruction = ir_section->first;
  int generated_label;
  struct ir_instruction *temp_instruction;
  bool label_found = false;

  while(instruction->next != NULL) {
    if(instruction->kind == IR_GENERATED_LABEL) {
      label_found = false;
      assert(instruction->operands[0].kind == OPERAND_GENERATED_LABEL);
      generated_label = instruction->operands[0].data.generated_label;
      temp_instruction = instruction->next;
      while(temp_instruction != NULL) {
	if((temp_instruction->kind == IR_GOTO) &&
	   (temp_instruction->operands[0].data.generated_label == generated_label)) {
	  label_found = true;
	  break;
	} else if(((temp_instruction->kind == IR_GOTO_IF_FALSE) ||
		  (temp_instruction->kind == IR_GOTO_IF_TRUE)) &&
		  (temp_instruction->operands[1].data.generated_label == generated_label)) {
	  label_found = true;
	  break;
	}
	temp_instruction = temp_instruction->next;
      }

      if(label_found == false) {
	temp_instruction = instruction->prev;
	while(temp_instruction != NULL) {
	  if((temp_instruction->kind == IR_GOTO) &&
	     (temp_instruction->operands[0].data.generated_label == generated_label)) {
	    label_found = true;
	    break;
	  } else if(((temp_instruction->kind == IR_GOTO_IF_FALSE) ||
		     (temp_instruction->kind == IR_GOTO_IF_TRUE)) &&
		    (temp_instruction->operands[1].data.generated_label == generated_label)) {
	    label_found = true;
	    break;
	  }
	  temp_instruction = temp_instruction->prev;
	}
      }

      if(label_found == false) {
	ir_remove_next_instruction(instruction->prev);
      }
    }
    instruction = instruction->next;
  }

  *root_ir = ir_section;
}

struct basic_block* add_to_basic_block(struct ir_instruction *beginning,
				       struct ir_instruction *end) {
  struct basic_block *basic_block;

  basic_block = malloc(sizeof(struct basic_block *));
  basic_block->ir_section = ir_section(beginning, end);
  basic_block->beginning = beginning;
  basic_block->beginning = end;
  basic_block->next = NULL;
  basic_block->left = NULL;
  basic_block->right = NULL;

  return basic_block;
}

struct basic_block * get_basic_blocks_from_ir(struct ir_section *root_ir) {
  struct ir_instruction *instruction = root_ir->first;

  struct ir_instruction *beginning_of_basic_block = root_ir->first;

  struct ir_instruction *end_of_basic_block = NULL;

  struct basic_block *root_basic_block = NULL, *basic_block = NULL;
  bool basic_block_found = false;
  int instruction_number = 0;

  while(instruction->next != NULL) {
    if((instruction->kind == IR_GOTO) ||
       (instruction->kind == IR_GOTO_IF_FALSE) ||
       (instruction->kind == IR_GOTO_IF_TRUE) ||
       (instruction->kind == IR_FUNCTION_END) ||
       (instruction->next->kind == IR_GENERATED_LABEL)) {
      end_of_basic_block = instruction;
      basic_block_found = true;
    }

    if(basic_block_found) {
      if(root_basic_block == NULL) {
	root_basic_block = add_to_basic_block(beginning_of_basic_block,
					      end_of_basic_block);
      } else {
	basic_block = root_basic_block;
	while(basic_block->next != NULL) {
	basic_block = basic_block->next;
	}
	basic_block->next = add_to_basic_block(beginning_of_basic_block,
					       end_of_basic_block);
      }
      beginning_of_basic_block = end_of_basic_block->next;
      end_of_basic_block = NULL;
      basic_block_found = false;
    }
    instruction = instruction->next;
    instruction_number++;
  }

  /* The last basic block */
    if(root_basic_block == NULL) {
      root_basic_block = add_to_basic_block(beginning_of_basic_block,
					    end_of_basic_block);
    } else {
      basic_block = root_basic_block;
      while(basic_block->next != NULL) {
	basic_block = basic_block->next;
      }
      basic_block->next = add_to_basic_block(beginning_of_basic_block,
					     end_of_basic_block);
    }
    return root_basic_block;
}

void propagate_constant_values(struct ir_section **root_ir) {
  struct basic_block *basic_block;

  basic_block = get_basic_blocks_from_ir(*root_ir);



}
