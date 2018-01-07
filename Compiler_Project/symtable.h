
#ifndef __SYMBOLTABLE_H__
#define __SYMBOLTABLE_H__


#include <bitset>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>

#include "auxlib.h"
#include "astree.h"






enum { ATTR_void, ATTR_int, ATTR_null,
       ATTR_string, ATTR_struct, ATTR_array, ATTR_function,
       ATTR_variable, ATTR_field, ATTR_typeid, ATTR_param,
       ATTR_lval, ATTR_const, ATTR_vreg, ATTR_vaddr,
       ATTR_bitset_size,
};
using attr_bitset = bitset<ATTR_bitset_size>;


struct astree;
using symbol_table = unordered_multimap<const string*, astree*>;
using symbol_entry = pair<string*,astree*>;
using astree_queue = queue<astree*>;


//Create the Symbol Table 
void make_struct_symbol(astree* root);
void make_block(astree* root);
void make_vardecl(astree* root);
void make_func_symbol(astree* root);
void make_proto_func(astree* root);
void traverse_block(astree* root);
void make_array(astree* root);

//Helper functions for Debugging
bool is_symbol (string token, astree* sym);
void print_stack(vector<symbol_table*> symStack);
void test_table();
void output_sym(const string* key, astree* sym);
astree* variable_exists(astree* root);

//Type Checking 
void type_check_block(astree* root) ;
bool check_struct_symbol(astree* root);
bool check_vardecl(astree* root);
bool check_proto_func(astree* root);
bool check_func_symbol(astree* root);
bool check_call(astree* root);
bool check_arguments(astree* root, astree*second);
bool check_types(astree* n, astree* s);
bool check_asg(astree* root);
bool check_bin_arithmetic(astree* root);
bool check_un_arithmetic(astree* root);
bool check_var(astree* root);
bool check_return(astree* root);
bool check_eq(astree* root);
bool check_select( astree* root);
bool check_index(astree* root);


#endif