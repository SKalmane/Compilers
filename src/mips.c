#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "node.h"
#include "type.h"
#include "symbol.h"
#include "ir.h"
#include "mips.h"

#define REG_EXHAUSTED   -1

#define FIRST_USABLE_REGISTER  8
#define LAST_USABLE_REGISTER  23
#define NUM_REGISTERS         32

/*
 * Change the IR to print the address of an identifier, not the name of the identifier
 * Create IR_NO_OPERATION instruction if we see an error
 * Simple optimization: multiply power of 2 - shiftleft that many bits
 * load immediate can be replaced by 'binary operation immediate'
 * add of 0(multiply of 1) can be replaced by no-op (need to remember the register correctly)
 * multiply of 0 can be replaced by no-op
 */

/****************************
 * MIPS TEXT SECTION OUTPUT *
 ****************************/

void mips_print_temporary_operand(FILE *output, struct ir_operand *operand) {
  assert(OPERAND_TEMPORARY == operand->kind);

  fprintf(output, "%8s%02d", "$", operand->data.temporary + FIRST_USABLE_REGISTER);
}

void mips_print_number_operand(FILE *output, struct ir_operand *operand) {
  assert(OPERAND_NUMBER == operand->kind);

  fprintf(output, "%10lu", operand->data.number);
}

void mips_print_identifier_operand(FILE *output, struct ir_operand *operand) {
  assert(OPERAND_IDENTIFIER == operand->kind);

  fprintf(output, "%s", operand->data.identifier.identifier_name);
}

void mips_print_generated_label(FILE *output, struct ir_operand *operand) {
  assert(OPERAND_GENERATED_LABEL == operand->kind);

  fprintf(output, "__GeneratedLabel_%04d", operand->data.generated_label);
}

void mips_print_generated_string_label(FILE *output, struct ir_operand *operand) {
  assert(OPERAND_STRING == operand->kind);

  fprintf(output, "__GeneratedStringLabel_%04d", operand->data.string_label.generated_label);
}

void mips_print_arithmetic(FILE *output, struct ir_instruction *instruction) {
    /* 3 operand R-Type instructions
     * add rdest, rsource1, rsource2
     * addu rdest, rsource1, rsource2
     * sub
     * subu
     * slt (set on less than) - compare as signed int, return 1 if true, 0 if false
     * sltu
     * and
     * or
     * xor
     * nor
     * srl rd, rt, sa (shift right logical by sa bits)
     * sra rd, rt, sa (shift right by sa, sign extending)
     * srlv rd, rt, rs (shift right by no of last 5 bits in rs)
     * srav rd, rt, rs (shift right by no of last 5 bits in rs, sign extending)
     * sll (shift left logical)
     * sllv (shift left logical variable)
     */

    /* Immediate Instructions
     * addi rt, rs, imm (imm is sign-extended and added to rs, stored in rt)
     * addiu
     * slti (set on less than immediate)
     * sltiu
     * andi (bitwise AND immediate)
     * ori
     * xori
     * lui (load upper immediate)
     */

    /* Branch instructions
     * beq rs, rt, offset  (branch on equal)
     * bne
     * blez rs, offset
     * bgtz
     * bltz
     * bgez
     * bltzal (branch on less than zero and link - used to call functions)
     * bgezal
     */

    /* Jump instructions
     * j
     * jal (jump and link)
     * jalr (jump and link register)
     */

    /* Load and store
     * lb rt, offset(base) (load byte)
     * lbu
     * lh
     * lhu
     * lw
     *
     */
    /* Mult/divide Instructions
     * mult op1, op2
     * multu
     * div
     * divu
     * mfhi rd (move from high register)
     * mfli rd (move from low register)
     * mthi rd (move to high register)
     * mtli rd
     */

  static char *opcodes[] = {
    NULL,
    NULL,
    NULL,
    NULL,
    "addu",
    "subu",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "sltu",
    "sle",
    "sgt",
    "sge",
    "sll",
    "sra",
    "seq",
    "sne",
    "or",
    "xor",
    "and",
    NULL
  };
  fprintf(output, "%10s ", opcodes[instruction->kind]);
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fputs(", ", output);
  mips_print_temporary_operand(output, &instruction->operands[1]);
  fputs(", ", output);
  mips_print_temporary_operand(output, &instruction->operands[2]);
  fputs("\n", output);
}

void mips_print_copy(FILE *output, struct ir_instruction *instruction) {
  fprintf(output, "%10s ", "or");
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fputs(", ", output);
  mips_print_temporary_operand(output, &instruction->operands[1]);
  fprintf(output, ", %10s\n", "$0");
}

void mips_print_load_immediate(FILE *output, struct ir_instruction *instruction) {
  fprintf(output, "%10s ", "li");
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fputs(", ", output);
  mips_print_number_operand(output, &instruction->operands[1]);
  fputs("\n", output);
}

void mips_print_print_number(FILE *output, struct ir_instruction *instruction) {
  /* Print the number. */
  fprintf(output, "%10s %10s, %10s, %10d\n", "ori", "$v0", "$0", 1);
  fprintf(output, "%10s %10s, %10s, ", "or", "$a0", "$0");
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fprintf(output, "\n%10s\n", "syscall");

  /* Print a newline. */
  fprintf(output, "%10s %10s, %10s, %10d\n", "ori", "$v0", "$0", 4);
  fprintf(output, "%10s %10s, %10s", "la", "$a0", "newline");
  fprintf(output, "\n%10s\n", "syscall");
}

bool need_to_allocate_memory_for_identifier(char local_variables[10][MAX_IDENTIFIER_LENGTH],
                                            char *identifier,
                                            int *number_of_identifiers_stored) {
    int i = 0;
    bool string_already_present = false;
    while(i < 10) {
        if(!strcmp(local_variables[i], identifier)) {
            string_already_present = true;
            break;
        }
        i++;
    }
    if(!string_already_present) {
        strcpy(local_variables[*number_of_identifiers_stored], identifier);
    }
    (*number_of_identifiers_stored)++;
    return !string_already_present;
}

void mips_print_function(FILE *output, struct ir_instruction *instruction) {
    struct ir_instruction *temp_instruction = instruction;

    /* To start off, we need storage space for:
     * 1.  s0 to s7 (32 bytes),
     * a0 - a3 (16 bytes)
     * the old stack frame pointer $fp (4 bytes)
     * the return address $ra (4 bytes)
     * one reserved word (4 bytes)
     * The minimum space needed = 60 bytes
     */
    int number_of_bytes_for_frame = 60;
    int word_aligned_number_of_bytes;

    /* Now, find out the additional of memory needed for the function */
    while(temp_instruction->kind != IR_FUNCTION_END) {
        temp_instruction = temp_instruction->next;
        if((temp_instruction->kind == IR_ADDRESS_OF) &&
           (temp_instruction->operands[1].kind == OPERAND_IDENTIFIER)) {
	  number_of_bytes_for_frame +=
	    temp_instruction->operands[1].data.identifier.symbol->owner_symbol_table->total_stack_offset;
	  break;
        }
    }

    word_aligned_number_of_bytes = (((number_of_bytes_for_frame + 7) >> 3) << 3);

    /* First, print out the label corresponding to this function name in the
     * mips file */
    fputs("\n", output);
    mips_print_identifier_operand(output, &instruction->operands[0]);
    fputs(":\n", output);

    /* Print all the stack frame related instructions */
    fprintf(output, "%10s %10s, %10s, %10d\n", "addi", "$sp", "$sp", -(word_aligned_number_of_bytes));
    /* Store the old frame pointer */
    fprintf(output, "%10s %10s, %10s\n", "sw", "$fp", "52($sp)");
    /* Save the return address */
    fprintf(output, "%10s %10s, %10s\n", "sw", "$ra", "56($sp)");
    /* Set the new frame pointer */
    fprintf(output, "%10s %10s, %10s, %10s\n", "or", "$fp", "$sp", "$0");
    /* Save the passed in parameters */
    fprintf(output, "%10s %10s, %10s\n", "sw", "$a0", "4($fp)");
    fprintf(output, "%10s %10s, %10s\n", "sw", "$a1", "8($fp)");
    fprintf(output, "%10s %10s, %10s\n", "sw", "$a2", "12($fp)");
    fprintf(output, "%10s %10s, %10s\n", "sw", "$a3", "16($fp)");

    /* Save the s-registers */
    fprintf(output, "%10s %10s, %10s\n", "sw", "$s0", "20($fp)");
    fprintf(output, "%10s %10s, %10s\n", "sw", "$s1", "24($fp)");
    fprintf(output, "%10s %10s, %10s\n", "sw", "$s2", "28($fp)");
    fprintf(output, "%10s %10s, %10s\n", "sw", "$s3", "32($fp)");
    fprintf(output, "%10s %10s, %10s\n", "sw", "$s4", "36($fp)");
    fprintf(output, "%10s %10s, %10s\n", "sw", "$s5", "40($fp)");
    fprintf(output, "%10s %10s, %10s\n", "sw", "$s6", "44($fp)");
    fprintf(output, "%10s %10s, %10s\n", "sw", "$s7", "48($fp)");

}

void mips_print_load_address(FILE *output, struct ir_instruction *instruction) {
  int stack_offset = 60;
  fprintf(output, "%10s ", "la");
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fputs(", ", output);
  if(instruction->operands[1].kind == OPERAND_IDENTIFIER) {
      stack_offset += instruction->operands[1].data.identifier.symbol->stack_offset;
      fprintf(output, "   %d($fp)\n", stack_offset);
  } else {
      assert(instruction->operands[1].kind == OPERAND_STRING);
      mips_print_generated_string_label(output, &instruction->operands[1]);
      fprintf(output, "\n");
  }
}

void mips_print_load_word(FILE *output, struct ir_instruction *instruction) {
  fprintf(output, "%10s ", "lw");
  mips_print_temporary_operand(output, &instruction->operands[0]);
  assert(OPERAND_TEMPORARY == instruction->operands[1].kind);
  fputs(", ", output);
  fprintf(output, "%9s%02d", "0($", instruction->operands[1].data.temporary + FIRST_USABLE_REGISTER);
  fprintf(output, ")\n");
}

void mips_print_store_word(FILE *output, struct ir_instruction *instruction) {
  fprintf(output, "%10s ", "sw");
  mips_print_temporary_operand(output, &instruction->operands[1]);
  assert(OPERAND_TEMPORARY == instruction->operands[1].kind);
  fputs(", ", output);
  fprintf(output, "%8s%02d", "0($", instruction->operands[0].data.temporary + FIRST_USABLE_REGISTER);
  fprintf(output, ")\n");
}

void mips_print_function_parameter(FILE *output, struct ir_instruction *instruction) {
  fprintf(output, "%10s ", "or");
  fprintf(output, "%9s%lu", "$a", instruction->operands[0].data.number);
  fputs(", ", output);
  mips_print_temporary_operand(output, &instruction->operands[1]);
  fprintf(output, ", %10s\n", "$0");
}

void mips_print_function_call(FILE *output, struct ir_instruction *instruction) {
  /* Save all the t registers*/
    char *function_name = instruction->operands[0].data.identifier.identifier_name;
    bool isSysFcnCall = (!strcmp(function_name, "print_int") ||
                         !strcmp(function_name, "read_int") ||
                         !strcmp(function_name, "read_string") ||
                         !strcmp(function_name, "print_string"));
    if(!isSysFcnCall) {
        fprintf(output, "%10s ", "jal");
        fprintf(output, "%10s\n", function_name);
    } else {
        fprintf(output, "%10s ", "li");
        if(!strcmp(function_name, "print_int")) {
            fprintf(output, "%10s ", "$v0, ");
            fprintf(output, "%10s ", "1");
        } else if(!strcmp(function_name, "print_string")) {
            fprintf(output, "%10s ", "$v0, ");
            fprintf(output, "%10s ", "4");
        } else if(!strcmp(function_name, "read_string")) {
            /* xxx: The reads have more work to be done */
        } else if(!strcmp(function_name, "read_int")) {
            fprintf(output, "%10s ", "$v0, ");
            fprintf(output, "%10s ", "5");
        } else {
            printf("Not a system function. Should not come here\n");
            assert(0);

        }
        fprintf(output, "\n%10s\n", "syscall");
    }
}

void mips_print_result_word(FILE *output, struct ir_instruction *instruction) {
    assert(IR_RESULTWORD == instruction->kind);
    fprintf(output, "%10s ", "or");
    mips_print_temporary_operand(output, &instruction->operands[0]);
    fprintf(output, ",");
    fprintf(output, "%12s", "$v0,");
    fprintf(output, "%11s\n", "$0");
}

void mips_print_goto_if_false(FILE *output, struct ir_instruction *instruction) {
    assert(IR_GOTO_IF_FALSE == instruction->kind);
    fprintf(output, "%10s ", "beqz");
    mips_print_temporary_operand(output, &instruction->operands[0]);
    fprintf(output, ",");
    mips_print_generated_label(output, &instruction->operands[1]);
    fprintf(output, "\n");
}

void mips_print_goto_if_true(FILE *output, struct ir_instruction *instruction) {
    assert(IR_GOTO_IF_TRUE == instruction->kind);
    fprintf(output, "%10s ", "bnez");
    mips_print_temporary_operand(output, &instruction->operands[0]);
    fprintf(output, ",");
    mips_print_generated_label(output, &instruction->operands[1]);
    fprintf(output, "\n");
}

void mips_print_goto(FILE *output, struct ir_instruction *instruction) {
    assert(IR_GOTO == instruction->kind);
    fprintf(output, "%10s ", "b");
    mips_print_generated_label(output, &instruction->operands[0]);
    fprintf(output, "\n");
}

void mips_print_label(FILE *output, struct ir_instruction *instruction) {
    assert(IR_GENERATED_LABEL == instruction->kind);
    fprintf(output, "\n");
    mips_print_generated_label(output, &instruction->operands[0]);
    fprintf(output, ":\n");
}

void mips_print_return(FILE *output, struct ir_instruction *instruction) {
    assert(IR_RETURN == instruction->kind);
    fprintf(output, "%10s ", "or");
    fprintf(output, "%11s", "$v0,");
    mips_print_temporary_operand(output, &instruction->operands[0]);
    fprintf(output, ",");
    fprintf(output, "%11s\n", "$0");
}

void mips_print_function_end(FILE *output, struct ir_instruction *instruction) {
    struct ir_instruction *temp_instruction = instruction;
    int number_of_bytes_for_frame = 60;
    int word_aligned_number_of_bytes;
    assert(IR_FUNCTION_END == instruction->kind);

    /* Restore the s-registers */
    fprintf(output, "%10s %10s, %10s\n", "lw", "$s7", "48($fp)");
    fprintf(output, "%10s %10s, %10s\n", "lw", "$s6", "44($fp)");
    fprintf(output, "%10s %10s, %10s\n", "lw", "$s5", "40($fp)");
    fprintf(output, "%10s %10s, %10s\n", "lw", "$s4", "36($fp)");
    fprintf(output, "%10s %10s, %10s\n", "lw", "$s3", "32($fp)");
    fprintf(output, "%10s %10s, %10s\n", "lw", "$s2", "28($fp)");
    fprintf(output, "%10s %10s, %10s\n", "lw", "$s1", "24($fp)");
    fprintf(output, "%10s %10s, %10s\n", "lw", "$s0", "20($fp)");

    /* Restore the return address */
    fprintf(output, "%10s %10s, %10s\n", "lw", "$ra", "56($sp)");

    /* Restore the old frame pointer */
    fprintf(output, "%10s %10s, %10s\n", "lw", "$fp", "52($sp)");

    /* Now, find out the additional of memory needed for the function */
    while(temp_instruction->kind != IR_FUNCTION_BEGIN) {
        temp_instruction = temp_instruction->prev;
        if(temp_instruction == NULL) {
            printf("There should be a procbegin instruction here\n");
            assert(0);
        }
        if((temp_instruction->kind == IR_ADDRESS_OF) &&
           (temp_instruction->operands[1].kind == OPERAND_IDENTIFIER)) {
            number_of_bytes_for_frame +=
                temp_instruction->operands[1].data.identifier.symbol->owner_symbol_table->total_stack_offset;
            break;
        }
    }
    word_aligned_number_of_bytes = (((number_of_bytes_for_frame + 7) >> 3) << 3);

    /* Pop off the stack frame */
    fprintf(output, "%10s %10s, %10s, %10d\n", "addi", "$sp", "$sp", (word_aligned_number_of_bytes));

    /* Return to caller */
    fprintf(output, "%10s %10s\n\n", "jr", "$ra");
}

void mips_print_multiply_or_divide(FILE *output, struct ir_instruction *instruction) {
    if(IR_MULTIPLY == instruction->kind) {
        fprintf(output, "%10s ", "multu");
    } else if(IR_DIVIDE == instruction->kind) {
        fprintf(output, "%10s ", "divu");
    } else {
        assert(IR_REMAINDER == instruction->kind);
        fprintf(output, "%10s ", "divu");
    }
    mips_print_temporary_operand(output, &instruction->operands[1]);
    fprintf(output, ",");
    mips_print_temporary_operand(output, &instruction->operands[2]);
    fprintf(output, "\n");

    if(instruction->kind == IR_MULTIPLY) {
        /* Retrieving the lower 32 bits from the LO register.
         * Here, we assume that there is no overflow due to
         * multiply. This can be dangerous and is a short-term
         * fix
         */
        fprintf(output, "%10s ", "mflo");
        mips_print_temporary_operand(output, &instruction->operands[0]);
        fprintf(output, "\n");
    }

    if(instruction->kind == IR_DIVIDE) {
        /* The quotient is stored in the LO register */
        fprintf(output, "%10s ", "mflo");
        mips_print_temporary_operand(output, &instruction->operands[0]);
        fprintf(output, "\n");
    }

    if(instruction->kind == IR_REMAINDER) {
        /* The quotient is stored in the HI register */
        fprintf(output, "%10s ", "mfhi");
        mips_print_temporary_operand(output, &instruction->operands[0]);
        fprintf(output, "\n");
    }
}

void mips_print_bitwise_not(FILE *output, struct ir_instruction *instruction) {
    fprintf(output, "%10s ", "not");
    mips_print_temporary_operand(output, &instruction->operands[0]);
    fprintf(output, ",");
    mips_print_temporary_operand(output, &instruction->operands[1]);
    fprintf(output, "\n");
}

void mips_print_negation(FILE *output, struct ir_instruction *instruction) {
    fprintf(output, "%10s ", "not");
    mips_print_temporary_operand(output, &instruction->operands[0]);
    fprintf(output, ",");
    mips_print_temporary_operand(output, &instruction->operands[1]);
    fprintf(output, "\n");

    fprintf(output, "%10s ", "addi");
    mips_print_temporary_operand(output, &instruction->operands[0]);
    fprintf(output, ",");
    mips_print_temporary_operand(output, &instruction->operands[0]);
    fprintf(output, ", %10d\n", -1);
    fprintf(output, "\n");
}

void mips_print_logical_not(FILE *output, struct ir_instruction *instruction) {
    fprintf(output, "%10s ", "sne");
    mips_print_temporary_operand(output, &instruction->operands[0]);
    fprintf(output, ",");
    mips_print_temporary_operand(output, &instruction->operands[0]);
    fprintf(output, ",");
    fprintf(output, "%10s\n", "$0");
    fprintf(output, "\n");
}

void mips_print_instruction(FILE *output, struct ir_instruction *instruction) {
  switch (instruction->kind) {
    case IR_ADD:
    case IR_SUBTRACT:
    case IR_LESS_THAN:
    case IR_LESS_THAN_OR_EQ_TO:
    case IR_GREATER_THAN:
    case IR_GREATER_THAN_OR_EQ_TO:
    case IR_SHIFT_LEFT:
    case IR_SHIFT_RIGHT:
    case IR_EQUAL_TO:
    case IR_NOT_EQUAL_TO:
    case IR_BITWISE_OR:
    case IR_BITWISE_XOR:
    case IR_BITWISE_AND:
      mips_print_arithmetic(output, instruction);
      break;

    case IR_MULTIPLY:
    case IR_DIVIDE:
    case IR_REMAINDER:
      mips_print_multiply_or_divide(output, instruction);
      break;
    case IR_COPY:
      mips_print_copy(output, instruction);
      break;

    case IR_BITWISE_NOT:
      mips_print_bitwise_not(output, instruction);
      break;

    case IR_LOGICAL_NOT:
      mips_print_logical_not(output, instruction);
      break;

    case IR_NEGATION:
      mips_print_negation(output, instruction);
      break;

    case IR_LOAD_IMMEDIATE:
      mips_print_load_immediate(output, instruction);
      break;

    case IR_PRINT_NUMBER:
      mips_print_print_number(output, instruction);
      break;

    case IR_FUNCTION_BEGIN:
      mips_print_function(output, instruction);
      break;
    case IR_FUNCTION_END:
      mips_print_function_end(output, instruction);
      break;
    case IR_FUNCTION_CALL:
      mips_print_function_call(output, instruction);
      break;
  case IR_ADDRESS_OF:
      mips_print_load_address(output, instruction);
      break;
  case IR_LOAD_WORD:
      mips_print_load_word(output, instruction);
      break;
  case IR_STORE_WORD:
      mips_print_store_word(output, instruction);
      break;
  case IR_FUNCTION_PARAMETER:
      mips_print_function_parameter(output, instruction);
      break;
  case IR_RESULTWORD:
      mips_print_result_word(output, instruction);
      break;
    case IR_GOTO_IF_FALSE:
      mips_print_goto_if_false(output, instruction);
      break;
    case IR_GOTO_IF_TRUE:
      mips_print_goto_if_true(output, instruction);
      break;
    case IR_GOTO:
      mips_print_goto(output, instruction);
      break;
    case IR_GENERATED_LABEL:
      mips_print_label(output, instruction);
      break;
    case IR_RETURN:
      mips_print_return(output, instruction);
      break;
    case IR_NO_OPERATION:
      break;

    default:
      printf("Kind of IR Instruction: %d\n", instruction->kind);
      assert(0);
      break;
  }
}

void print_string(FILE *output, char *str) {
    int i = 0;
    fputs("\"", output);
    do {
        if (str[i] == 0) {
            fputs("\\0", output);
        } else if(str[i] == '\\') {
            fputs("\\", output);
        } else if(str[i] == '\n') {
            fputs("\\n", output);
        } else {
            fprintf(output, "%c", str[i]);
        }
        i++;
    } while(str[i] != '\0');
    fputs("\"", output);
}

void mips_print_string_labels(FILE *output, struct ir_section *section) {
    struct ir_instruction *instruction;
    for (instruction = section->first; instruction != section->last->next; instruction = instruction->next) {
        if(instruction->operands[1].kind == OPERAND_STRING) {
          fprintf(output, "\n__GeneratedStringLabel_%04d: .asciiz ",
                  instruction->operands[1].data.string_label.generated_label);
          print_string(output, instruction->operands[1].data.string_label.name);
      }
    }
    fputs("\n", output);
}

void mips_print_text_section(FILE *output, struct ir_section *section) {
  struct ir_instruction *instruction;
  fputs("\n.data", output);
  mips_print_string_labels(output, section);

  fputs("\n.text\n.globl main\n", output);

  for (instruction = section->first; instruction != section->last->next; instruction = instruction->next) {
    mips_print_instruction(output, instruction);
  }

  /* Return from main. */
  fprintf(output, "\n%10s %10s\n", "jr", "$ra");
}

void mips_print_program(FILE *output, struct ir_section *section) {
  mips_print_text_section(output, section);
}
