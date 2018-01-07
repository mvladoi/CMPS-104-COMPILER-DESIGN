#include <bitset>
#include <iostream>
#include <string>
#include <unordered_map>

#include "symtable.h"
#include "astree.h"
#include "lyutils.h"
#include "string_set.h"




vector<symbol_table*> sym_stack(100, nullptr);
vector<int> blockCounter(1,0);
int nextBlock = 1;
FILE* sym_output;
astree_queue string_queue;
astree_queue prototype_queue;
astree_queue struct_queue;
astree_queue globalv_queue;
astree_queue function_queue;


////////////////////////////////////////////////////////////////////////
// Create a new_symbol caling the copy constructor 
// from the astree class, return a symbol with all 
// the properties of "node"
////////////////////////////////////////////////////////////////////////
astree* newsymbol(astree* node)
{
    astree* a = node->new_symbol(node);
    a->blockNum = blockCounter.back();
    node->blockNum = blockCounter.back();
    return a;
}



/////////////////////////////////////////////////////////////////////
//Create a symbol in the sym_stack table 
//If the pointer is nullptr, create another symbol_table
//and insert the symbol 
//////////////////////////////////////////////////////////////////////
void insert_symbol (int pos, const string* key,
                    astree* sym)
{
    if(!sym_stack.at(pos))
    {
        sym_stack.at(pos) = new symbol_table();

    }
    sym_stack.at(pos)->insert(make_pair(key,sym));
}


////////////////////////////////////////////////////////////////////////
//Create a new block, increase the block number , 
//store the block number, assign to the symbol the block 
//number and traverse the block  
////////////////////////////////////////////////////////////////////////
void make_block (astree* root)
{
    root->blockNum = blockCounter.back();
    blockCounter.push_back(nextBlock);
    nextBlock++;
    traverse_block(root);
    blockCounter.pop_back();
}



//////////////////////////////////////////////////////////////////////////
//Create a struct_symbol, store all the fields of the 
//struct and insert the struct in the sym_stack at 
//pos 0 (locality)
//////////////////////////////////////////////////////////////////////////
void make_struct_symbol (astree* root)
{
    astree* sym = newsymbol(root->children[0]);
    const string* key;
    key = root->children[0]->lexinfo;
    sym->type_id = root->children[0]->lexinfo->c_str();
    sym->fields = new symbol_table();
    output_sym(key,sym);
    if(root->children.size() > 1)
    {
        astree* fieldlist = root->children[1];
        for (size_t index = 0;
                index < fieldlist->children.size(); ++index)
        {
            astree* field = fieldlist->children[index]->children[0];
            const string* key2 = field->lexinfo;
            if(is_symbol("TOK_TYPEID",fieldlist->children[index]))
            {   fieldlist->children[index]->children[0]->type_id=
		         fieldlist->children[index]->lexinfo->c_str();
            }
			astree* sym2 = newsymbol(field);
			sym2->field_name = root->children[0]->lexinfo->c_str();
            fprintf (sym_output, "   ");
            sym->fields->insert(make_pair(key2,sym2));
            output_sym(key2,sym2);
        }
    }
	
	struct_queue.push(sym);
    if (check_struct_symbol(sym)) {
    insert_symbol(0, key, sym);
	}
}


//////////////////////////////////////////////////////////////////////////////
//Type check a struct for redefinition 
///////////////////////////////////////////////////////////////////////////////
bool check_struct_symbol(astree* root) {
	int pos = 0;
	if(!sym_stack.at(pos)){
	    return true;  
	}
	
    auto got = sym_stack.at(pos)->find(root->lexinfo);
    if (got == sym_stack.at(pos)->end()){
        return true;
	}
	fprintf(stderr,  "conflicting types, structure redefinition for  '%s'", 
     root->lexinfo->c_str());
    fprintf(stderr, "location (%lu.%lu.%lu)\n",
     root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
    return false;
}


///////////////////////////////////////////////////////////////////////////
//Print the symbol in the .sym  output file
/////////////////////////////////////////////////////////////////////////// 
void output_sym (const string* key, astree* sym)
{

    for (size_t size = 1; size < blockCounter.size(); ++size)
    {
        fprintf(sym_output, "   ");
    }
    fprintf (sym_output, "%s (%zu.%zu.%zu) {%d} %s\n", key->c_str(),
             sym->lloc.filenr, sym->lloc.linenr, sym->lloc.offset, 
			 sym->blockNum,
             sym->check_attr (sym));
}

////////////////////////////////////////////////////////////////////////
//Helper function to check if the "sym" has the name "token"
///////////////////////////////////////////////////////////////////////
bool is_symbol (string token, astree* sym){
	return (token ==
            string(parser::get_tname(sym->symbol)));
	
}





//////////////////////////////////////////////////////////////////////////////
bool is_lexinfo(string token, astree* root){
	
	if (token == root->lexinfo->c_str())
		return  true;
	return false;
	
	
}


/////////////////////////////////////////////////////////////////////////
//Create a func_symbol, set the attributes, set all the parameters of the 
//function, type check the function and insert it to the sym_stack
//////////////////////////////////////////////////////////////////////////
void make_func_symbol(astree* root)
{

    astree* function = root->children.at(0)->children.at(0);
    astree* param_node  = root->children.at(1);
    astree* block_node  = root->children.at(2);
	function->attributes[ATTR_variable] = 0;
    function->attributes[ATTR_lval] = 0;
    function->attributes.set(ATTR_function);
    astree* sym = newsymbol(function);
	sym->type_id = root->children[0]->lexinfo->c_str();
    const string* key;
    key = function->lexinfo;
    sym->parameters = new vector<astree*>();
    output_sym(key,sym);

    for (size_t index = 0; index < param_node->children.size(); 
	     ++index)
    {
		astree* param;
		 if(is_symbol("TOK_ARRAY",param_node->children[index])){
			param = param_node->children[index]->children[1]; 
			 
		 }
		 else {
			 param = param_node->children[index]->children[0];
		 }
		
        const string* key2 = param->lexinfo;
        if(is_symbol("TOK_TYPEID",param_node->children[index]))
        {
        param_node->children[index]->children[0]->type_id = 
		      param_node->children[index]->lexinfo->c_str();
        }
		
        astree* sym2 = newsymbol(param);
        sym->parameters->push_back(sym2);
        fprintf(sym_output, "   ");
        insert_symbol(0, key2, sym2);
        output_sym(key2,sym2);
    }
    
	 if (check_func_symbol(sym)){
    insert_symbol (0, key, sym);
    fprintf(sym_output, "\n");
    make_block(block_node);
	function_queue.push(root);
	}
}

/////////////////////////////////////////////////////////////////////////////
//Check the function. Look in the sym_stack if there is a function prototype
//with the same name. 
//Check if have the same number of parameters and if the 
//parameteres have the same type
////////////////////////////////////////////////////////////////////////////
bool check_func_symbol(astree* root){
   int pos = blockCounter.back();
 
	if(!sym_stack.at(pos)){
	    return true;  
	}
	
	astree* sym;
    symbol_table* table = sym_stack.at(pos);
    auto got = table->find(root->lexinfo);
	
    if (got == sym_stack.back()->end()) {
		return true;
	
	}
	
	

	else {	
       sym = got->second;
       attr_bitset attributes(sym->attributes);
         
		   if(root->parameters and sym->parameters){
                if (root->parameters->size() != sym->parameters->size()) {
				   fprintf(stderr,  "conflicting types for  '%s'", 
                       root->lexinfo->c_str());
                   fprintf(stderr, "location (%lu.%lu.%lu)\n",
                 root->lloc.filenr, root->lloc.linenr, root->lloc.offset);		
                return false; 
				} 
		    }
		    else if ( (!root->parameters) or (!sym->parameters)){
				   fprintf(stderr, "conflicting types for '%s'", 
                       root->lexinfo->c_str());
                   fprintf(stderr, "location (%lu.%lu.%lu)\n",
                  root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
			    return false;
		    }
		    else if (attributes != root->attributes) {
			        fprintf(stderr, "conflicting attributes types for '%s'", 
                       root->lexinfo->c_str());
                   fprintf(stderr, "location (%lu.%lu.%lu)\n",
               root->lloc.filenr, root->lloc.linenr, root->lloc.offset);	
               return false;
		    }
                  
            else { 
			    for (unsigned i = 0; i < root->parameters->size(); i++) {
                         if (root->parameters->at(i)->attributes 
                         != sym->parameters->at(i)->attributes) { 
						 
				    fprintf(stderr, " parameters conflicting types for '%s'", 
                       root->lexinfo->c_str());
                    fprintf(stderr, "location (%lu.%lu.%lu)\n",
                 root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
						 
                 return false;
                        }
					  
				    }
			 }  					
			 
			 
				     
					  return true;

        }	
	
	
	
	
}

///////////////////////////////////////////////////////////////////////////
//Helper function to check if two symbold have the same type
//////////////////////////////////////////////////////////////////////////
bool check_types(astree* n, astree* s) {
    attr_bitset attr1 = n->attributes;
    attr_bitset attr2 = s->attributes;
    if ((attr1.test(ATTR_null) &&  (attr2.test(ATTR_string)
                                 || attr2.test(ATTR_struct)
                                 || attr2.test(ATTR_array)))
      || (attr2.test(ATTR_null) &&  (attr1.test(ATTR_string)
                                 || attr1.test(ATTR_struct)
                                 || attr1.test(ATTR_array))))
        return true;
    if ((attr1.test(ATTR_array) && !attr2.test(ATTR_array))
      || (attr1.test(ATTR_array) && !attr2.test(ATTR_array)))
        return false;
    if (attr1.test(ATTR_int)
       && attr2.test(ATTR_int)) {
        return true;
    } else if (attr1.test(ATTR_string)
       && attr2.test(ATTR_string)) {
        return true;
    } else if (attr1.test(ATTR_struct)
       && attr2.test(ATTR_struct)) {
        if (n->struct_name == s->struct_name)
            return true;
    }
    return false;
}



//////////////////////////////////////////////////////////////////////////////
//Check a function call by looking in the sym_stack if the function exists
//Check if the arguments of the function call are defined. Check if the 
//arguments have the same type as the argumets of the function prototype
//////////////////////////////////////////////////////////////////////////////
bool check_call(astree* root){
	
	int pos = 0;
	if(!sym_stack.at(pos)){
	    return false;  
	}
	
	astree* sym;
    symbol_table* table = sym_stack.at(pos);
	
	if (!is_lexinfo("getargv", root->children.at(0))){
    auto got = table->find(root->children.at(0)->lexinfo);
	
    if (got == table->end()) {
		fprintf(stderr, "function '%s' not found " , 
		root->children.at(0)->lexinfo->c_str());
        fprintf(stderr, "location (%lu.%lu.%lu)\n",
        root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
		return false;
	}
	
	else{
		sym = got->second;
		unsigned  root_length = root->children.at(1)->children.size();
		unsigned  sym_length = sym->parameters->size();

		if(root_length != sym_length){
			fprintf(stderr, "function '%s' has wrong dimensions ", 
			root->children.at(0)->lexinfo->c_str());
            fprintf(stderr, "location (%lu.%lu.%lu)\n",
             root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
			 return false;
			
		}
		else if (root_length == 0){
			return true;
		}
		
		else{
			 for (unsigned j = 0; j < root_length -1; j++) {
				 astree* first = root->children.at(1)->children.at(j);
				 astree* second = sym->parameters->at(j);
				 if (!check_arguments(first,second)){			 
                      fprintf(stderr, "improper function call '%s' ", 
					 root->children.at(0)->lexinfo->c_str());
                      fprintf(stderr, "location (%lu.%lu.%lu)\n",
                    root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
				     return false;
		            }
		
			    }
		
	        }
	
	
	    }

		
		
		root->attributes = sym->attributes;
	   return true;
	}
	return true;
}
  



////////////////////////////////////////////////////////////////////////////
//Helper function to check if an argument of a call function is in the 
//sym_stack
//If the argumet is found, check if it has the same type with "second",
// which is the same argumet from the function prototype
////////////////////////////////////////////////////////////////////////////
 bool check_arguments(astree* root, astree* second) { 
    symbol_table *table;
	int low = 0;
	if (is_symbol("TOK_IDENT", root)){
    for(int i = int (sym_stack.size() - 1); i >= low; i--) {
		if ( sym_stack.at(i) != nullptr){
          table = sym_stack.at(i);
          for  (auto got = table->begin(); got != table->end(); got++) {
            if (got->first == root->lexinfo) {
				  if (check_types(second, got->second) )
				     return true;
				
            }
           }
		}
    }
    fprintf(stderr, "unknown argument '%s' send to function ",
            root->lexinfo->c_str());
    fprintf(stderr, "location (%lu.%lu.%lu)\n", root->lloc.filenr,
            root->lloc.linenr, root->lloc.offset);
	return false;
    }
   return true;	
}
  
  
  
 
////////////////////////////////////////////////////////////////////////////
//Create prototype function symbol, set the parameters and insert it in the 
//sym_stack
////////////////////////////////////////////////////////////////////////////
void make_proto_func(astree* root)
{
    astree* function = root->children.at(0)->children.at(0);
    astree* param_node  = root->children.at(1);
	function->attributes[ATTR_variable] = 0;
    function->attributes[ATTR_lval] = 0;
    function->attributes.set(ATTR_function);
    astree* sym = newsymbol(function);
	sym->type_id = root->children[0]->lexinfo->c_str();
    const string* key;
    key = function->lexinfo;
    sym->parameters = new vector<astree*>();
    output_sym(key,sym);


    for (size_t index = 0; index < param_node->children.size(); ++index)
    {
        astree* param = param_node->children[index]->children[0];
        const string* key2 = param->lexinfo;
        if(is_symbol("TOK_TYPEID",param_node->children[index]))
        {   param_node->children[index]->children[0]->type_id = 
		      param_node->children[index]->lexinfo->c_str();
        }
        astree* sym2 = newsymbol(param);
        sym->parameters->push_back(sym2);
        fprintf(sym_output, "   ");
        insert_symbol(0, key2, sym2);
        output_sym(key2,sym2);
    }
    if (check_proto_func(sym)){
    insert_symbol (0, key, sym);
    fprintf(sym_output, "\n");
	prototype_queue.push(sym);
	}
}




/////////////////////////////////////////////////////////////////////////////
//Type check the prototype function. Make sure that it was not defined before 
//in the sym_stack (redefinition) 
// //////////////////////////////////////////////////////////////////////////
bool check_proto_func(astree* root) {
   int pos = blockCounter.back();

	if(!sym_stack.at(pos)){
	    return true;  
	}

	astree* sym;
    symbol_table* table = sym_stack.at(pos);
    auto got = table->find(root->lexinfo);
	
    if (got == sym_stack.back()->end()) {
		return true;

	}
	
	

	else {	
       sym = got->second;
       attr_bitset attributes(sym->attributes);
         
		   if(root->parameters and sym->parameters){
                if (root->parameters->size() != sym->parameters->size()) {
                return true; 
				} 
		    }
		    else if ( (!root->parameters) or (!sym->parameters)){
			    return true;
		    }
		    else if (attributes != root->attributes) {
            return true;
		    }
                  
            else { 
			    for (unsigned i = 0; i < root->parameters->size(); i++) {
                         if (root->parameters->at(i)->attributes 
                         != sym->parameters->at(i)->attributes) { 
                         return true;
                        }
					  
				    }
			 }  					
			 
			 
				      fprintf(stderr, "redefinition of '%s'", root->lexinfo->c_str());
                      fprintf(stderr, "location (%lu.%lu.%lu) ",
                      root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
					  return false;

        }
 }


///////////////////////////////////////////////////////////////////////////
//Create a variable symbol, check the variable and insert it to the sym_stack
////////////////////////////////////////////////////////////////////////////// 
void make_vardecl(astree* root)
{
    astree* sym = nullptr; 
    const string* key = nullptr;
	root->children[0]->children[0]->blockNum = blockCounter.back();
	if( is_symbol("TOK_ARRAY", root->children[0]))
    {
		sym = newsymbol(root->children[0]->children[1]);
        root->children[0]->children[1]->type_id = root->children[0]->lexinfo->c_str();
		key = sym->lexinfo;
    }
	
	
	else {
	sym = newsymbol(root->children[0]->children[0]);
	key = sym->lexinfo;
    if(is_symbol("TOK_TYPEID", root->children[0]))         
    {
		sym = newsymbol(root->children[0]->children[0]);
		root->children[0]->children[0]->type_id = root->children[0]->lexinfo->c_str();
        sym->type_id = root->children[0]->lexinfo->c_str();
    }
	
	}
	if (check_vardecl(sym)){
     insert_symbol (blockCounter.back(),key, sym);
     output_sym(key,sym);
	 if(blockCounter.back() == 0){
		globalv_queue.push(sym);
	 }
    }
}


///////////////////////////////////////////////////////////////////////////
//Check a variable for redefinition
///////////////////////////////////////////////////////////////////////////
bool check_vardecl(astree* root) {
int pos = blockCounter.back();
  if (sym_stack.at(pos)){	
    auto got = sym_stack.at(pos)->find(root->lexinfo);
    if (got == sym_stack.at(pos)->end()) {
		return true;
    } else {
		
		astree*  sym = got->second;
        attr_bitset attributes(sym->attributes);
		
	if(sym->attributes != root->attributes){
		return true;
	    }
		
    else{ 
		fprintf(stderr, " variable redefinition of '%s' ",
                root->lexinfo->c_str());
        fprintf(stderr, "location (%lu.%lu.%lu)\n",
                root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
        return false;				
        }
    }
  }
  return true;
}


/////////////////////////////////////////////////////////////////////////////
//Helper function to print the sym_stack, used for testing
//////////////////////////////////////////////////////////////////////////// 
void print_stack(vector<symbol_table*> sym_stack)
{
    int depth = 0;
    for ( unsigned int i = 0; i < sym_stack.size(); i++)
    {

        if(sym_stack.at(i))
        {
            symbol_table* temp = sym_stack[i];
            for ( auto it =  temp->begin(); it != temp->end(); ++it )
            {
                cout << " " << *(it->first)   << " bucket " << i <<   endl;
                it->second-> really_prints_ast(it->second,depth);
                if (it->second->fields)
                {
                    cout  << it->second->fields->size()  << " fields " << endl;
                    for ( auto itt =  it->second->fields->begin(); 
					itt != it->second->fields->end(); ++itt )
                    {
                        cout << *(itt->first)  << endl;
                        itt->second-> really_prints_ast(itt->second,depth+2);
                    }
                }
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////
//Call the print_stack, 
////////////////////////////////////////////////////////////////////////////
void test_table()
{
    print_stack(sym_stack);
}





///////////////////////////////////////////////////////////////////////////
//Helper function used to check if an identifier is in the symbol table
///////////////////////////////////////////////////////////////////////////
astree* variable_exists(astree* root){
    int low = 0;   
	int pos = sym_stack.size()-1;
	
	  for(int i = pos; i >= low; i--){	
	        if(sym_stack.at(i)){
             symbol_table* table = sym_stack.at(i);
             auto got = table->find(root->lexinfo);
	
                if (got !=sym_stack.back()->end()) {
		             return got->second;
                     				 
				
                   
	            } 	
	
	
            }
	    }		
		return nullptr;
}



/////////////////////////////////////////////////////////////////////////
//Check if the variable is in the symbol table 
////////////////////////////////////////////////////////////////////////
bool check_var(astree* root){
	
	astree * sym;
	 if((!is_symbol("TOK_INTCON",root))
	    and (!is_lexinfo("null",root))
		and (!is_lexinfo("[",root))
		and (!is_lexinfo("getargv",root))){
	sym = variable_exists(root);
		if (sym){
		         root->attributes = sym->attributes;
                 root->attributes[ATTR_param] = 0;
                 root->blockNum = sym->blockNum;				 
			 
		}
		
		
		else {fprintf(stderr, "identifier '%s ' not declared ," , 
			 root->lexinfo->c_str());
             fprintf(stderr, "location (%lu.%lu.%lu)\n",
             root->lloc.filenr, root->lloc.linenr, root->lloc.offset); 
			 }
	 }	

    if (is_lexinfo ("getargv", root)){
		root->attributes.set(ATTR_int);
		insert_symbol(blockCounter.back(), root->lexinfo, root);
		return true;
		
	}	 
			
	return true;
}




///////////////////////////////////////////////////////////////////////////////
//Check if the l value has the attribute set 
//Check if the l value is compatible with r value
///////////////////////////////////////////////////////////////////////////////
bool check_asg(astree* root) { 

    if ((!root->children.at(0)->attributes.test(ATTR_lval))){
        fprintf(stderr, "assignment to immutable element '%s' ",
		root->children.at(0)->lexinfo->c_str());
        fprintf(stderr, "location (%lu.%lu.%lu)\n",
                root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
        return false;
    }
	
	return true;
	
	
}

///////////////////////////////////////////////////////////////////////////////
//Check to perform binary arithmetic operations only on int values 
///////////////////////////////////////////////////////////////////////////////

bool check_bin_arithmetic(astree* root) { 

   	for (unsigned i = 0; i < 2; i++){
		
			 if ( !root->children.at(i)->attributes.test(ATTR_int)){
		        fprintf(stderr, "incompatible binary arithmetic operation," 
			     "'%s' value not an int  ", 
			    root->children.at(i)->lexinfo->c_str());
                fprintf(stderr, "location (%lu.%lu.%lu)\n",
                root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
                return false; 
				}
	}				
   
    root->attributes.set(ATTR_vreg);
	root->attributes.set(ATTR_int);
	return true;
   
}



////////////////////////////////////////////////////////////////////////////
//Check the return statement, it handles only the case when we return a 
//string or an integer
///////////////////////////////////////////////////////////////////////////
bool check_return(astree* root){

    bool sym = check_var(root->children.at(0));
		if (sym)
        return true;	
		   	 
		 
		
	
         fprintf(stderr, "incompatible return statement," 
	    "'%s'  ", 
	    root->lexinfo->c_str());
        fprintf(stderr, "location (%lu.%lu.%lu)\n",
        root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
	
	return false;
}



//////////////////////////////////////////////////////////////////////////
//Check if both values used in equality test exists and if they have the 
//same type 
//////////////////////////////////////////////////////////////////////////

bool check_eq(astree* root){
	
	bool sym1 = check_var(root->children.at(0));
	bool sym2 = check_var(root->children.at(1));
	
	if (sym1 and sym2){ 
	
        if (check_types(root->children.at(0), root->children.at(1))) {
             root->attributes.set(ATTR_vreg);
			 return true;
    }   else {
          fprintf(stderr, "incompatible equality  statement, " 
	      "'%s' ", 
	      root->lexinfo->c_str());
          fprintf(stderr, "location (%lu.%lu.%lu)\n",
          root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
	   	  return false;
        }         
	}
	else {
		
		  fprintf(stderr, "one of the variable in eq statement , " 
	      "'%s'  was not declared ", 
	      root->lexinfo->c_str());
          fprintf(stderr, "location (%lu.%lu.%lu)\n",
          root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
	   	  return false;
		
	}
	
}






///////////////////////////////////////////////////////////////////////////
//Check if both values used in comparison test exists and if they have the 
//same type 
///////////////////////////////////////////////////////////////////////////

bool check_comp( astree* root){
	
	
	bool sym1 = check_var(root->children.at(0));
	bool sym2 = check_var(root->children.at(1));
	
	if (sym1 and sym2){ 
	
        if (check_types(root->children.at(0), root->children.at(1))) {
             root->attributes.set(ATTR_vreg);
			 return true;
			}
		else{
		  fprintf(stderr, "incompatible comparison  statement, " 
	      "'%s' ", 
	      root->lexinfo->c_str());
          fprintf(stderr, "location (%lu.%lu.%lu)\n",
          root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
	   	  return false;
		}	 
	        
	}
		  fprintf(stderr, "one of the variable in eq statement , " 
	      "'%s'  was not declared ", 
	      root->lexinfo->c_str());
          fprintf(stderr, "location (%lu.%lu.%lu)\n",
          root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
	   	  return false;	
    
 
}


///////////////////////////////////////////////////////////////////////////
//Check the Select operator. Check if the struct exists on the symbol table. 
//Check if the struct has the field that is needed.  
///////////////////////////////////////////////////////////////////////////
bool check_select( astree* root){
	
	astree* sym_id = root->children.at(0);
	astree* sym_field = root->children.at(1);
	astree* sym_struct = variable_exists(sym_id);
	
	if (!sym_struct){
	
		fprintf(stderr, "struct type , " 
	      "'%s'  was not declared ", 
	      root->lexinfo->c_str());
          fprintf(stderr, "location (%lu.%lu.%lu)\n",
          root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
		  return false;
		
	}
	
     else if (!sym_id->attributes.test(ATTR_struct)) {
	
        fprintf(stderr, "accesing not struct type , " 
	      "'%s'  was not declared ", 
	      root->lexinfo->c_str());
          fprintf(stderr, "location (%lu.%lu.%lu)\n",
          root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
        return false;
    }
	
	
	else {
		
		if (sym_struct->fields){
        auto got = sym_struct->fields->find(sym_field->lexinfo);
	
	            if (got == sym_struct->fields->end()) {

				fprintf(stderr, "accesing invalid struct field type , " 
	            "'%s'  ", 
	            root->lexinfo->c_str());
                fprintf(stderr, "location (%lu.%lu.%lu)\n",
                root->lloc.filenr, root->lloc.linenr, 
			    root->lloc.offset);
                return false;
                     }
	            else{ 
	            sym_field->attributes = got->second->attributes;
	            root->attributes = got->second->attributes;
	            root->attributes.set(ATTR_vaddr);
	            root->attributes.set(ATTR_lval);
                root->attributes.set(ATTR_field, 0);
		        return true;
		        }
	    
		
		}else{
			
			fprintf(stderr, "accesing invalid struct field type , " 
	       "'%s'  ", 
	       root->lexinfo->c_str());
           fprintf(stderr, "location (%lu.%lu.%lu)\n",
           root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
           return false;
			
		}
	}
	 return true;
	
	
	
}



////////////////////////////////////////////////////////////////////////////
//Helper function 
////////////////////////////////////////////////////////////////////////////
void make_array(astree* root) { 
  insert_symbol(blockCounter.back(), root->children.at(1)->lexinfo,
                 root->children.at(1));
  
}









////////////////////////////////////////////////////////////////////////////
//Traverse the parse tree, create the symbol table 
////////////////////////////////////////////////////////////////////////////
void traverse_block(astree* root) {
  
  
   for (size_t i = 0; i < root->children.size(); i++) {
	   
      int node_symbol = root->children[i]->symbol;
      astree* _root =  root->children[i];
  
  
    switch (node_symbol) {
        case TOK_ROOT:
            sym_stack.at(0) = new symbol_table();
            break;
        case TOK_STRUCT:
            make_struct_symbol(_root);
            fprintf(sym_output, "\n");
            break;
        case TOK_VARDECL:
            make_vardecl(_root);
            break;
        case TOK_FUNCTION:
            make_func_symbol(_root);
            fprintf(sym_output, "\n");
            break;
        case TOK_IF:
            make_block(_root->children[1]);
            break;
		case TOK_WHILE:
            make_block(_root->children[1]);
            break;
        case TOK_IFELSE:
            make_block(_root->children[1]);
            make_block(_root->children[2]);
            break;	
        case TOK_PROTOTYPE:
            make_proto_func(_root);
            fprintf(sym_output, "\n");
            break;	
		case TOK_ARRAY:
            make_array(_root);
            break;
        case TOK_STRINGCON:
            string_queue.push(_root);
            break;			

			
    }
  
   }
   
}



//////////////////////////////////////////////////////////////////////
//After creating the symbol table typecheck the code 
//////////////////////////////////////////////////////////////////////
void type_check_block(astree* root) {
	
	
	 for (size_t child = 0; child < root->children.size(); child++) {
	         type_check_block (root->children[child]);
	    }
		
		switch (root->symbol) {
       
        case TOK_CALL:
           check_call(root);
            break;
		case TOK_IDENT:
            check_var(root);
            break;
		case '+':
        case '-':
        case '*':
        case '/':
        case '%':
           check_bin_arithmetic(root);
            break;	
        case '=':
            check_asg(root);
            break;	
        case TOK_RETURN:
            check_return(root);
			break;
        case TOK_EQ:
        case TOK_NE:
            check_eq(root);
            break;
        case TOK_LT:
        case TOK_LE:
        case TOK_GT:
        case TOK_GE:
            check_comp(root);
            break;      
        case '.':
            check_select(root);
            break;
    }
		
    
	
  }










