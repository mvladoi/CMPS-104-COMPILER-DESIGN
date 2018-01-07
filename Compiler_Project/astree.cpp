// $Id: astree.cpp,v 1.8 2016-09-21 17:13:03-07 - - $

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "astree.h"
#include "string_set.h"
#include "lyutils.h"
#include "symtable.h"

FILE* ast_output;




astree::astree (){
   symbol = 0;
   attributes = 0;
   blockNum = 0;
   struct_name = nullptr;
   fields = nullptr;
   lexinfo = nullptr;
   parameters = nullptr;
}


astree::astree (int symbol_, const location& lloc_, const char* info) {
   symbol = symbol_;
   lloc = lloc_;
   attributes = 0;
   blockNum = 0;
   struct_name = nullptr;
   fields = nullptr;
   lexinfo = string_set::intern(info);
   parameters = nullptr;
   // vector defaults to empty -- no children
}



 astree* astree:: new_symbol(astree* node){
	 
   astree* a = new astree();
   a->symbol = node->symbol;
   a->attributes = node->attributes;
   a->lexinfo = node->lexinfo;
   a->parameters = node->parameters;
   a->lloc = node->lloc;
   a->fields = node->fields;
   return a;
	 }

astree::~astree() {
   while (not children.empty()) {
      astree* child = children.back();
      children.pop_back();
      delete child;
   }
   if (yydebug) {
      fprintf (stderr, "Deleting astree (");
      astree::dump (stderr, this);
      fprintf (stderr, ")\n");
   }
}

astree* astree::adopt (astree* child1, astree* child2, astree* child3) {
   //cout << "In adopt: ";
   if (child1 != nullptr){
        children.push_back (child1);
        //cout << "Child 1 " << parser::get_tname(child1->symbol);
   }
   if (child2 != nullptr){
        children.push_back (child2);
        //cout << " Child 2 " << parser::get_tname(child2->symbol);
   }
   if (child3 != nullptr){
        children.push_back (child3);
        //cout << " Child 3 " << parser::get_tname(child3->symbol);
   }
   //cout << endl;
   //astree::debug_print(parser::root,0);
   //astree::really_prints_ast();
   return this;
}






astree* astree::adopt_sym (astree* child, int symbol_) {
   //cout << "AdoptSym" << endl;
   symbol = symbol_;
   return adopt (child);
}




astree* astree::swap_sym (astree* root, int symbol) {
    root->symbol = symbol;
    return root;
}

void astree::dump_node (FILE* outfile) {
   fprintf (outfile, "%p->{%s %zd.%zd.%zd \"%s\":",
            this, parser::get_tname (symbol),
            lloc.filenr, lloc.linenr, lloc.offset,
            lexinfo->c_str());
   for (size_t child = 0; child < children.size(); ++child) {
      fprintf (outfile, " %p", children.at(child));
   }
}

void astree::dump_tree (FILE* outfile, int depth) {
   fprintf (outfile, "%*s", depth * 3, "");
   dump_node (outfile);
   fprintf (outfile, "\n");
   for (astree* child: children) child->dump_tree (outfile, depth + 1);
   fflush (NULL);
}

void astree::dump (FILE* outfile, astree* tree) {
   if (tree == nullptr) fprintf (outfile, "nullptr");
                   else tree->dump_node (outfile);
}

void astree::print (FILE* outfile, astree* tree, int depth) {
   const char *tname = parser::get_tname(tree->symbol);
   if (strstr (tname, "TOK_") == tname) tname += 4;
   for (int i = 0; i < depth; i++) {
      fprintf(outfile, "|   ");
   }
   //fprintf (outfile, "| %*s", depth * 3, "|");
   fprintf (outfile, "%s \"%s\" (%zd.%zd.%zd){%d} %s\n",
            tname, tree->lexinfo->c_str(),
            tree->lloc.filenr, tree->lloc.linenr, tree->lloc.offset,
			tree->blockNum, tree->check_attr(tree));
   for (astree* child: tree->children) {
      astree::print (outfile, child, depth + 1);
   }
   //cout << "Printing: " << tname << endl;
}





void astree:: really_prints_ast(astree* tree, int depth){
	
	
	const char *tname = parser::get_tname(tree->symbol);
   if (strstr (tname, "TOK_") == tname) tname += 4;
   for (int i = 0; i < depth; i++) {
      cout <<  "|   ";
   }
   //fprintf (outfile, "| %*s", depth * 3, "|");
  cout << tname << " " <<  tree->lexinfo->c_str() << " " << 
         tree->lloc.filenr << " " <<  tree->lloc.linenr << " " <<   tree->lloc.offset << " " <<
			tree->blockNum   << " " << tree->check_attr(tree) << endl;
   for (astree* child: tree->children) {
      astree::really_prints_ast (child, depth + 1);
   }
	
	
	
	
	
};

void astree::debug_print (astree* tree, int depth) {
   printf ("; %*s", depth * 5, "");
   printf ("%s \"%s\" (%zd.%zd.%zd)  \n",
            parser::get_tname (tree->symbol), tree->lexinfo->c_str(),
            tree->lloc.filenr, tree->lloc.linenr, tree->lloc.offset );
   for (astree* child: tree->children) {
      astree::debug_print(child, depth + 1);
   }
}

void destroy (astree* tree1, astree* tree2) {
   //cout << "Tree is being destroyed: ";
   if (tree1 != nullptr){ 
   //cout << "tree 1 " << parser::get_tname(tree1->symbol);
   delete tree1;}
   if (tree2 != nullptr){ 
   //cout << "tree 2 " << parser::get_tname(tree2->symbol);
   delete tree2;}

   //cout << endl;
}

void errllocprintf (const location& lloc, const char* format,
                    const char* arg) {
   static char buffer[0x1000];
   assert (sizeof buffer > strlen (format) + strlen (arg));
   snprintf (buffer, sizeof buffer, format, arg);
   errprintf ("%s:%zd.%zd: %s", 
              lexer::filename (lloc.filenr), lloc.linenr, lloc.offset,
              buffer);
}



 const char* astree::check_attr (astree* tree){
   
   string out = "";
   attr_bitset attr = tree-> attributes;
    if (attr.test(ATTR_void)) {
        out += "void ";}
    if (attr.test(ATTR_field)) {	
     out += "field {" + tree->field_name + "} ";}
    if (attr.test(ATTR_array)) {
        out += "array ";}
    if (attr.test(ATTR_int)) {
        out += "int ";}
    if (attr.test(ATTR_null)) {
        out += "null ";}
    if (attr.test(ATTR_string)) {
        out += "string ";}
    if (attr.test(ATTR_struct)) {
        out += "struct ";}
    if (attr.test(ATTR_typeid)) {
        out += "\"" + tree->type_id + "\" ";}
    if (attr.test(ATTR_function)) {
        out += "function ";}
    if (attr.test(ATTR_variable)) {
        out += "variable ";}
    if (attr.test(ATTR_lval)) {
        out += "lval ";}
    if (attr.test(ATTR_param)) {
        out += "param ";}
    if (attr.test(ATTR_const)) {
        out += "const ";}
    if (attr.test(ATTR_vreg)) {
        out += "vreg ";}
    if (attr.test(ATTR_vaddr)) {
        out += "vaddr ";}
 
   return out.c_str();
}







void  set_attribute(astree* node){
   switch(node->symbol)
   {
      case TOK_VOID:       node->attributes.set(ATTR_void);
                           if (node->children.size() > 0)
                           node->children[0]-> 
                              attributes.set(ATTR_void);
                           break;
      case TOK_INT:        node->attributes.set(ATTR_int); 
                           node->attributes.set(ATTR_lval); 
                           if (node->children.size() > 0)
                           node->children[0]-> 
                                 attributes.set(ATTR_int);
                           break;
      case TOK_NULL:       node->attributes.set(ATTR_null);
                           node->attributes.set(ATTR_const);
                           break;
      case TOK_STRING:     node->attributes.set(ATTR_string);
                           node->attributes.set(ATTR_lval);
                           if (node->children.size() > 0)
                           node->children[0]-> 
                                attributes.set(ATTR_string);
                           break;
      case TOK_STRUCT:     node->attributes.set(ATTR_struct);
                           node->attributes.set(ATTR_typeid);
                           node->children[0]-> 
                              attributes.set(ATTR_struct);
                           node->attributes.set(ATTR_variable);
                           node->attributes.set(ATTR_lval);
                           node->children[0]->
                              attributes[ATTR_variable] = 0;
                           node->children[0]->
                              attributes[ATTR_lval] = 0;
                           break;
      case TOK_ARRAY:      node->attributes.set(ATTR_array);
	                       switch(node->children.at(0)->symbol) {
                                case TOK_INT:
                                 node->attributes.set(ATTR_int);
                                 break;
                                case TOK_STRING:
                                 node->attributes.set(ATTR_string);
                                 break;
                                case TOK_TYPEID: 
                                node->attributes.set(ATTR_typeid);
                                node->struct_name = node->children.at(0)->lexinfo;
                                 break;
						   }
						   break;
                           
						   
      case TOK_FUNCTION:   node->children.at(0)->attributes.set(ATTR_function);
	                       node->children.at(0)->attributes[ATTR_variable] = 0; 
	                       if(node->children.size() == 2){
						   node->children.at(2)->children.at
						   (node->children.at(2)->children.size()-1)->
						   attributes = node->children.at(0)->attributes;
						   }
                           break;
    
      case TOK_ARGUMENTS:  for (size_t index = 0;
                                 index < node->children.size(); ++index)
							     node->children[index]->
                                 attributes.set(ATTR_param); 
                           break;
      case TOK_PARAMLIST:  for (size_t index = 0;
                                 index < node->children.size(); ++index)
                                 node->children[index]->children[0]->
                                 attributes.set(ATTR_param);
							break;	 
      case TOK_STRINGCON:  node->attributes.set(ATTR_string);
                           node->attributes.set(ATTR_const);
                           break;
      case TOK_INTCON:     node->attributes.set(ATTR_int);
                           node->attributes.set(ATTR_const);
                           break;
      case TOK_FIELD:      node->attributes.set(ATTR_field);
                           if("TOK_TYPEID" == 
                           string(parser::get_tname(node->symbol))){
                              node->attributes.set(ATTR_struct);}
                           break;
      case TOK_TYPEID:     node->attributes.set(ATTR_typeid);
                           for(size_t index = 0; 
                              index < node->children.size(); ++index)
                           {
                            node->children[index]->
                            attributes.set(ATTR_typeid);
                            node->children[index]->
                            attributes.set(ATTR_struct);
                           break;
                      }
      case TOK_IDENT:      node->attributes.set(ATTR_lval);
                           node->attributes.set(ATTR_variable);
                           break;
      case TOK_VARDECL:    node->attributes.set(ATTR_variable);
                           break;
      case TOK_DECLID:     node->attributes.set(ATTR_lval);
                           node->attributes.set(ATTR_variable);
                           break;
						   
      default:             break;
   }
}



void typecheck_array(astree* root) { 
        root->children.at(1)->attributes.set(ATTR_variable);
	    root->children.at(1)->attributes.set(ATTR_lval);
		root->children.at(1)->attributes.set(ATTR_array);


    switch(root->children.at(0)->symbol) {
        case TOK_INT:
            root->children.at(1)->attributes.set(ATTR_int);
            break;
        case TOK_STRING:
            root->children.at(1)->attributes.set(ATTR_string);
            break;
        case TOK_TYPEID: 
               root->children.at(1)->attributes.set(ATTR_typeid);
               root->children.at(1)->field_name = root->children.at(0)->lexinfo->c_str();
             break;
		default: break;	
    }
}



void run_attr(astree* root){
   for( size_t index = 0; index < root->children.size(); ++index){
      run_attr(root->children[index]);
   }
   set_attribute(root);
}












