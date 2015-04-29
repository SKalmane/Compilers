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

  fprintf(output, "%s", operand->data.identifier_name);
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
    "mulu",
    "divu",
    "addu",
    "subu",
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
    char local_variables[10][MAX_IDENTIFIER_LENGTH];
    int i, number_of_identifiers_stored = 0;

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

    for(i = 0; i < 10; i++) {
        strcpy(local_variables[i], "");
    }
    
    /* Now, find out the additional of memory needed for the function */
    while(temp_instruction->kind != IR_FUNCTION_END) {
        temp_instruction = temp_instruction->next;
        if((temp_instruction->kind == IR_ADDRESS_OF) &&
            (temp_instruction->operands[1].kind == OPERAND_IDENTIFIER)) {
            if(need_to_allocate_memory_for_identifier(
                   local_variables,
                   temp_instruction->operands[1].data.identifier_name,
                   &number_of_identifiers_stored)) {
                number_of_bytes_for_frame += 4;
            }
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

void mips_print_instruction(FILE *output, struct ir_instruction *instruction) {
  switch (instruction->kind) {
    case IR_MULTIPLY:
    case IR_DIVIDE:
    case IR_ADD:
    case IR_SUBTRACT:
      mips_print_arithmetic(output, instruction);
      break;

    case IR_COPY:
      mips_print_copy(output, instruction);
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

  mips_print_string_labels(output, section);

  fputs("\n.text\nmain:\n", output);

  for (instruction = section->first; instruction != section->last->next; instruction = instruction->next) {
    mips_print_instruction(output, instruction);
  }

  /* Return from main. */
  fprintf(output, "\n%10s %10s\n", "jr", "$ra");
}

void mips_print_program(FILE *output, struct ir_section *section) {
  mips_print_text_section(output, section);
}
