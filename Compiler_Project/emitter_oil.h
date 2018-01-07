

#ifndef __EMITTER_OIL__
#define __EMITTER_OIL__

#include <string>
#include <unordered_map>
#include <bitset>
#include <vector>
#include <queue>
#include <string>

#include "lyutils.h"
#include "symtable.h"
#include "astree.h"
#include "yyparse.h"

using namespace std;



struct astree;
using astree_queue = queue <astree*>;

//Function called from main 
void generate_code(astree* root);


//Helper Functions
astree* get_id(astree* root);
string make_oil_type(astree* root);
char register_name(astree* node); 



//Function to generate the oil code 
void generate_prototypes();
void generate_structs();
void generate_stringcons();
void generate_vardecls();
void generate_functions();
void generate_expr(astree* root);
void generate_stm(astree* root);
void generate_operand(astree* root);
void generate_bin_arithmetic(astree* root);
void generate_assignment(astree* root);
void generate_vardecl(astree* root);
void generate_while(astree* root);
void generate_compare(astree* root);
void generate_ifelse(astree* root);
void generate_if(astree* root);
void generate_return(astree* root);
void generate_negate(astree* root);
void generate_new( astree* root);
void generate_newarray(astree* root);
void generate_newstring(astree* root);
void generate_index(astree* root);
void generate_call(astree* root);
void generate_select(astree* root);


#endif