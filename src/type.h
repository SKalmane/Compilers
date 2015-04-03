#ifndef _TYPE_H
#define _TYPE_H

#include <stdio.h>
#include <stdbool.h>

struct node;

#define TYPE_BASIC    1
#define TYPE_VOID     2
#define TYPE_POINTER  3
#define TYPE_ARRAY    4
#define TYPE_FUNCTION 5
#define TYPE_LABEL    6

#define TYPE_WIDTH_CHAR     1
#define TYPE_WIDTH_SHORT    2
#define TYPE_WIDTH_INT      4
#define TYPE_WIDTH_LONG     4
#define TYPE_WIDTH_POINTER  4

#define CONVERSION_RANK_LONG                   50
#define CONVERSION_RANK_INT                    40
#define CONVERSION_RANK_SHORT                  30
#define CONVERSION_RANK_CHAR                   20

struct type {
  int kind;
  union {
    struct {
      bool is_unsigned;
      int width;
      int conversion_rank;
    } basic;
    struct {
      struct type *pointee;
    } pointer;
    struct {
        struct type *array_type;
        unsigned long array_size;
    } array;
    struct {
        struct type *return_type;
        struct symbol_list *parameter_list;
        int number_of_parameters;
        struct symbol_table *function_symbol_table;
        struct node *function_body;
    } function;
  } data;
};

struct type *type_basic(bool is_unsigned, int width, int conversion_rank);
struct type *type_void();
struct type *type_label();
struct type *type_pointer(struct type *pointee);
struct type *type_function(struct type *return_type);
struct type *type_array(struct type *array_type, unsigned long array_size);

int type_size(struct type *t);

int type_equal(struct type *left, struct type *right);

int type_is_arithmetic(struct type *t);
int type_is_unsigned(struct type *t);

void type_assign_in_statement_list(struct node *statement_list);

void type_assign_in_translation_unit(struct node *translation_unit);

void type_print(FILE *output, struct type *type);

extern FILE *error_output;
extern int type_checking_num_errors;

#endif /* _TYPE_H */
