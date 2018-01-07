// $Id: astree.h,v 1.7 2016-10-06 16:13:39-07 - - $


#ifndef __ASTREE_H__
#define __ASTREE_H__

#include <string>
#include <vector>
using namespace std;

#include "auxlib.h"
#include "symtable.h"











struct location {
   size_t filenr;
   size_t linenr;
   size_t offset;
};


struct astree;

using attr_bitset = bitset<15>;
using symbol_table = unordered_multimap<const string*, astree*>;

struct astree {

   // Fields.
   int symbol;               // token code
   location lloc;            // source location
   const string* lexinfo;    // pointer to lexical information
   vector<astree*> children; // children of this n-way node
   attr_bitset attributes;
   const string* struct_name;
   symbol_table* fields;
   vector<astree*>* parameters;
   int blockNum;
   string field_name;
   string type_id;
   int reg_num;
   string code;
   

   // Functions.
   void really_prints_ast(astree* tree, int depth = 0);
   astree();
   astree (int symbol, const location&, const char* lexinfo);
   ~astree();
   astree* adopt (astree* child1, astree* child2 = nullptr,\
                  astree* child3 = nullptr);
   astree* adopt_sym (astree* child, int symbol);
   astree* swap_sym (astree* root, int symbol);
   astree* new_symbol(astree* node);
   void dump_node (FILE*);
   void dump_tree (FILE*, int depth = 0);
   static void dump (FILE* outfile, astree* tree);
   static void print (FILE* outfile, astree* tree, int depth = 0);
   static void debug_print (astree* tree, int depth);
   const char* check_attr(astree* tree);
   
   
};
void set_attribute(astree* node);
void run_attr(astree* root);
void typecheck_array(astree* root);

void destroy (astree* tree1, astree* tree2 = nullptr);

void errllocprintf (const location&, const char* format, const char*);

#endif