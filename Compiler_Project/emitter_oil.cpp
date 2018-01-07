

#include <iostream>
#include "emitter_oil.h"


using namespace std;

int  reg_num = 1;
FILE* oil_output;





///////////////////////////////////////////////////////////////
void generate_code(astree* root) {
   
    generate_prototypes();
    generate_structs();
    generate_stringcons();
    generate_vardecls();
    generate_functions();
    generate_stm(root);
    
}






//////////////////////////////////////////////////////////////
string make_oil_type(astree* node) {
    string value = "";
    if (node->attributes.test(ATTR_void)) {
        value.append("void");
        return value;
    }

     if (node->attributes.test(ATTR_int)) {
        value.append("int");
    } else if (node->attributes.test(ATTR_string)) {
        value.append("char*");
    } else if (node->attributes.test(ATTR_struct)) {
        value.append("struct s_");
		if (node->struct_name)
        value.append(*(node->struct_name));
        value.append("*");
    }

    if (node->attributes.test(ATTR_array)) {
        value.append("*");
    }
    return value;
}





//////////////////////////////////////////////////////////////
void generate_prototypes(){
	
	
    while (!prototype_queue.empty()) {
        astree *sym  = prototype_queue.front();
        prototype_queue.pop();
        fprintf(oil_output, "%s __%s(",
                make_oil_type(sym).c_str(),
                sym->lexinfo->c_str());
        vector<astree*>* params = sym->parameters;
		if (params){
         for (unsigned i = 0; i < params->size(); i++) {
            fprintf(oil_output, "\n");
            fprintf(oil_output, "        ");
			
			if(params->at(i)){
            fprintf(oil_output, "%s _%d_%s",
                    make_oil_type(params->at(i)).c_str(),
                    params->at(i)->blockNum,
                    params->at(i)->lexinfo->c_str());
                if (i < params->size() - 1)
                      fprintf(oil_output, ",");
            }
		 }
        }
            fprintf(oil_output, ");\n");
       
    }
	
	fprintf(oil_output, "\n");
	
}






/////////////////////////////////////////////////////////////////////
void generate_structs(){
	
	while (!struct_queue.empty()) {
		astree *sym  = struct_queue.front();
        struct_queue.pop();
        fprintf(oil_output, "struct s_%s {\n", sym->lexinfo->c_str());
        symbol_table* field_table = sym->fields;
        for (auto it =  field_table->begin(); it != field_table->end();  
		     it++) {
            fprintf(oil_output, "        ");
            fprintf(oil_output, "%s f_%s_%s;\n",
                    make_oil_type(it->second).c_str(), 
                    sym->lexinfo->c_str(),
                    it->first->c_str());
        }
        fprintf(oil_output, "};\n\n");
    	
    }
}









/////////////////////////////////////////////////////////////////////
void generate_stringcons(){
	
	  while (!string_queue.empty()) {
        astree* sym  = string_queue.front();
        string_queue.pop();
        const string* value = sym->lexinfo;
        fprintf(oil_output, "char* s%d = %s;\n", reg_num, 
		         value->c_str());
        sym->reg_num = reg_num; reg_num++; 
	  }
		
}
   
   
   
   
   
   
//////////////////////////////////////////////////////////////////////   
void generate_vardecls(){
	
	while (!globalv_queue.empty() ) {
         astree* sym  = globalv_queue.front();
         globalv_queue.pop();
         fprintf(oil_output, "%s __%s;\n", make_oil_type(sym).c_str(),
                    sym->lexinfo->c_str());
       
    }
	
}








///////////////////////////////////////////////////////////////////////
void generate_functions(){
	
	
   while (!function_queue.empty()) {
        astree * node  = function_queue.front();
		astree * sym  = node->children.at(0);
        function_queue.pop();
        fprintf(oil_output, "%s __%s(",
                make_oil_type(sym).c_str(),
                sym->lexinfo->c_str());
        vector<astree*>* params = sym->parameters;
		if(params){
        for (unsigned i = 0; i < params->size(); i++) {
            fprintf(oil_output, "\n");
            fprintf(oil_output, "        ");
            fprintf(oil_output, "%s _%d_%s",
                    make_oil_type(params->at(i)).c_str(),
                    params->at(i)->blockNum,
                    params->at(i)->lexinfo->c_str());
            if (i < params->size() - 1)
                fprintf(oil_output, ",");
        }
		
		}
		
        
            fprintf(oil_output, ")\n");
            fprintf(oil_output, "{\n");
			if (node->children.size() == 3){
             generate_stm(node->children.at(2));
			}
            fprintf(oil_output, "}\n");
       
    }
	
	fprintf(oil_output, "\n");
}







////////////////////////////////////////////////////////////////////////
void generate_stm(astree* root) {
    switch (root->symbol) {
		case TOK_ROOT:
            fprintf(oil_output, "void __ocmain (void)\n");
            fprintf(oil_output, "{\n");
                 for (size_t child = 0; child < root->children.size(); 
				      child++) {
                       generate_stm(root->children[child]);
                    }
			fprintf(oil_output, "}\n");
            break;
        case TOK_FUNCTION:
            break;
		case TOK_VARDECL:
            generate_vardecl(root);
            break;
        case TOK_BLOCK:
                for (size_t child = 0; child < root->children.size(); 
				     child++) {
                       generate_stm(root->children[child]);
                    }
            break;
        case TOK_WHILE:
            generate_while(root);
            break;
		case TOK_IF:
            generate_if(root);
            break;	
        case TOK_IFELSE:
            generate_ifelse(root);
            break;
        case TOK_RETURN:
        case TOK_RETURNVOID:
            generate_return(root);
            break;
        default:
            generate_expr(root);
    }
	

}







/////////////////////////////////////////////////////////////////////
void generate_expr(astree* root) {
    for (size_t child = 0; child < root->children.size(); child++) {
        generate_expr(root->children[child]);
    }
    switch (root->symbol) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
            generate_bin_arithmetic(root);
            break;
        case '=':
            generate_assignment(root);
            break;
        case TOK_EQ:
        case TOK_NE:
        case TOK_LT:
        case TOK_LE:
        case TOK_GT:
        case TOK_GE:
            generate_compare(root);
            break;
        case '!':
            generate_negate(root);
            break;
        case TOK_NEW:
            generate_new(root);
            break;
        case TOK_NEWARRAY:
            generate_newarray(root);
            break;
        case TOK_NEWSTRING:
            generate_newstring(root);
            break;
        case TOK_INDEX:
            generate_index(root);
            break;
        case '.':
            generate_select(root);
            break;
        case TOK_CALL:
            generate_call(root);
            break;
    }
}




////////////////////////////////////////////////////////////////////////
char register_name (astree* node) {
    if (node->symbol == TOK_NEW
     || node->symbol == TOK_NEWARRAY
     || node->symbol == TOK_NEWSTRING) {
		
        return 'p';
    }
    if (node->attributes.test(ATTR_vaddr)
     || node->attributes.test(ATTR_array)) {
		 
        return 'a';
    }
    if (node->attributes.test(ATTR_int)) {
	
        return 'i';
    }   else {
		
        return 's';
    }
}








//////////////////////////////////////////////////////////////////////
void generate_operand(astree* root) {
    if (root->attributes.test(ATTR_vreg)
     ||   (root->attributes.test(ATTR_string)
        && root->attributes.test(ATTR_const))) {
        fprintf(oil_output, "%c%d", 
                register_name(root),
                root->reg_num);
    } else if (root->attributes.test(ATTR_vaddr)) {
        fprintf(oil_output, "*%c%d", 
                register_name(root),
                root->reg_num);
    } else if (root->attributes.test(ATTR_variable)) {
        fprintf(oil_output, "_");
        if (root->blockNum > 0)
            fprintf(oil_output, "%d", root->blockNum);
        fprintf(oil_output, "_%s", root->lexinfo->c_str());
    } else if (root->attributes.test(ATTR_const)) {
        string value = "";
        value.append(*(root->lexinfo));
        if (root->attributes.test(ATTR_int)) {
            while (value.at(0) == '0' && value.compare("0") != 0)
                value.erase(0, 1);
        }
        fprintf(oil_output, "%s", value.c_str());
    }
}






/////////////////////////////////////////////////////////////////////
void generate_bin_arithmetic(astree* root) {
	if(root->children.size() == 2){
    astree* root1 = root->children.at(0);
    astree* root2 = root->children.at(1);
    fprintf(oil_output, "        ");
    fprintf(oil_output, "int i%d = ", reg_num);
    root->reg_num = reg_num; reg_num++;
     generate_operand(root1);
    fprintf(oil_output, " %s ", root->lexinfo->c_str());
    generate_operand(root2);
    fprintf(oil_output, ";\n");
	}
}









//////////////////////////////////////////////////////////////////////
void generate_assignment(astree* root) {
    astree* ident = root->children.at(0);
    astree* expr  = root->children.at(1);
    fprintf(oil_output, "        ");
    if (ident->attributes.test(ATTR_variable)) {
        fprintf(oil_output, "_");
        if (ident->blockNum > 0)
            fprintf(oil_output, "%d", ident->blockNum);
        fprintf(oil_output, "_%s = ", ident->lexinfo->c_str());
    } else {
        generate_operand(ident);
        fprintf(oil_output, " = ");
    }
    generate_operand(expr);
    fprintf(oil_output, ";\n");
}









////////////////////////////////////////////////////////////////////////
astree* get_id(astree* root) {
    astree* sym = root->children.at(0);
    if (sym->symbol == TOK_ARRAY) {
        return sym->children.at(1);
    } else {
        return sym->children.at(0);
    }
}






////////////////////////////////////////////////////////////////////////
void generate_vardecl(astree* root) {
    astree* ident = get_id(root);
    astree* expr  = root->children.at(1);
    if (expr->attributes.test(ATTR_vreg) 
     || expr->attributes.test(ATTR_vaddr))
        generate_expr(expr);

    fprintf(oil_output, "        ");
    if (ident->blockNum > 0) {
        fprintf(oil_output, "%s _%d_%s = ",
               make_oil_type(ident).c_str(),
                ident->blockNum,
                ident->lexinfo->c_str());
    } else {
		
        fprintf(oil_output, " __%s = " ,ident->lexinfo->c_str());
    }
    generate_operand(expr);
    fprintf(oil_output, ";\n");
}






////////////////////////////////////////////////////////////////////////
void generate_while(astree* root){
    fprintf(oil_output, "while_%lu_%lu_%lu:;\n",
            root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
    generate_expr(root->children.at(0));
    fprintf(oil_output, "        ");
    fprintf(oil_output, "if (!i"); 
	fprintf(oil_output, to_string(root->children.at(0)->reg_num).c_str());
    fprintf(oil_output, ") goto break_%lu_%lu_%lu;\n",
            root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
    generate_stm(root->children.at(1));
    fprintf(oil_output, "        ");
    fprintf(oil_output, "goto while_%lu_%lu_%lu;\n",
            root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
    fprintf(oil_output, "break_%lu_%lu_%lu:;\n",
            root->lloc.filenr, root->lloc.linenr, root->lloc.offset);

}







//////////////////////////////////////////////////////////////////////////
void generate_compare(astree* root){
    astree* root1 = root->children.at(0);
    astree* root2 = root->children.at(1);
    fprintf(oil_output, "        ");
    fprintf(oil_output, "int i%d = ", reg_num);
    root->reg_num = reg_num;reg_num++; 	
    generate_operand(root1);
    fprintf(oil_output, " %s ", root->lexinfo->c_str());
    generate_operand(root2);
    fprintf(oil_output, ";\n");
}








/////////////////////////////////////////////////////////////////////////
void generate_if(astree* root){
	if((root->children.size() == 2)){
    generate_expr(root->children.at(0));
    fprintf(oil_output, "        ");
    fprintf(oil_output, "if (!i");
	fprintf(oil_output, to_string(root->children.at(0)->reg_num).c_str());
    fprintf(oil_output, ") goto fi_%lu_%lu_%lu;\n",
            root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
    generate_stm(root->children.at(1));
    fprintf(oil_output, "fi_%lu_%lu_%lu:;\n",
            root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
	}
}






/////////////////////////////////////////////////////////////////////////
void generate_ifelse(astree* root){
    generate_expr(root->children.at(0));
    fprintf(oil_output, "        ");
    fprintf(oil_output, "if (!i");
    fprintf(oil_output, to_string(root->children.at(0)->reg_num).c_str());
    fprintf(oil_output, ") goto else_%lu_%lu_%lu;\n",
            root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
    generate_stm(root->children.at(1));
    fprintf(oil_output, "        ");
    fprintf(oil_output, "goto fi_%lu_%lu_%lu;\n",
            root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
    fprintf(oil_output, "else_%lu_%lu_%lu:;\n",
            root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
    generate_stm(root->children.at(2));
    fprintf(oil_output, "fi_%lu_%lu_%lu:;\n",
            root->lloc.filenr, root->lloc.linenr, root->lloc.offset);
}







///////////////////////////////////////////////////////////////////////////
void generate_return(astree* root){
    fprintf(oil_output, "        ");
    if (root->symbol == TOK_RETURN) {
        generate_expr(root->children.at(0));
        fprintf(oil_output, "return ");
        generate_operand(root->children.at(0));
        fprintf(oil_output, ";\n");
    } else {
        fprintf(oil_output, "return;\n");
    }
}






////////////////////////////////////////////////////////////////////////////
void generate_negate(astree* root){
    astree* root1 = root->children.at(0);
    fprintf(oil_output, "        ");
    fprintf(oil_output, "char i%d = !", reg_num);
    root->reg_num = reg_num; reg_num++;
    generate_operand(root1);
    fprintf(oil_output, ";\n");
}









////////////////////////////////////////////////////////////////////////////
void generate_new(astree* root){
    astree* sym = root->children.at(0);
    fprintf(oil_output, "        ");
    fprintf(oil_output, "struct s_%s* p%d = ", 
            sym->lexinfo->c_str(), reg_num);
    root->reg_num = reg_num; reg_num++;
    fprintf(oil_output, "xcalloc (1, sizeof (struct s_%s));\n",
            sym->lexinfo->c_str());
}






////////////////////////////////////////////////////////////////////////////
void generate_newstring(astree* root){
    astree* size = root->children.at(0);
    fprintf(oil_output, "        ");
    fprintf(oil_output, "char* p%d = xcalloc (", reg_num);
    root->reg_num = reg_num; reg_num++;
    generate_operand(size);
    fprintf(oil_output, ", sizeof (char));\n");
}






//////////////////////////////////////////////////////////////////////
void generate_newarray(astree* root){
	
	if (root->children.size() ==2){
    astree* type   = root->children.at(0);
    astree* size   = root->children.at(1);
    string make_oiltype = "";
    switch (type->symbol) {
        case TOK_INT:
            make_oiltype.append("int");
            break;
        case TOK_STRING:
            make_oiltype.append("char*");
            break;
        case TOK_TYPEID:
            make_oiltype.append("struct s_");
            make_oiltype.append(*(type->struct_name));
            make_oiltype.append("*");
            break;
    }
    fprintf(oil_output, "        ");
    fprintf(oil_output, "%s* p%d = ",
            make_oiltype.c_str(),
            reg_num);
    root->reg_num = reg_num; reg_num++;
    fprintf(oil_output, "xcalloc (");
    generate_operand(size);
    fprintf(oil_output, ", sizeof (%s));\n",
            make_oiltype.c_str());
 }
}
 
 
 
 
 
 
 
////////////////////////////////////////////////////////////////////
void generate_index(astree* root) {
    astree* ident = root->children.at(0);
    astree* index = root->children.at(1);
    fprintf(oil_output, "        ");
    fprintf(oil_output, "%s a%d = ", 
            make_oil_type(ident).c_str(), reg_num);
    root->reg_num = reg_num; reg_num++;
    if (ident->attributes.test(ATTR_vaddr)
     || (ident->attributes.test(ATTR_vreg)
      && ident->attributes.test(ATTR_array))) {
        fprintf(oil_output, "&a%d", ident->reg_num);
    } else {
        fprintf(oil_output, "&_");
        if (ident->blockNum > 0)
            fprintf(oil_output, "%d", ident->blockNum);
        fprintf(oil_output, "_%s", ident->lexinfo->c_str());
    }

    fprintf(oil_output, "[");
    generate_operand(index);
    fprintf(oil_output, "]");
    fprintf(oil_output, ";\n");
}








/////////////////////////////////////////////////////////////////////////
void generate_call(astree* root){
    astree* ident = root->children.at(0);
    fprintf(oil_output, "        ");
    if (!root->attributes.test(ATTR_void)) {
        fprintf(oil_output, "%s %d = ", 
                make_oil_type(ident).c_str(),reg_num);
        root->reg_num = reg_num; reg_num++;
    }
    fprintf(oil_output, "__%s(", ident->lexinfo->c_str());
    for (unsigned i = 1; i < root->children.size(); i++) {
        generate_operand(root->children.at(i));
        if (i < root->children.size() - 1)
            fprintf(oil_output, ",");
    }
    fprintf(oil_output, ");\n");
}







//////////////////////////////////////////////////////////////////////////
void generate_select(astree* root){
	if (root->children.size() == 2){
    astree* ident = root->children.at(0);
    astree* field = root->children.at(1);
    fprintf(oil_output, "        ");
    fprintf(oil_output, "%s* a%d = ", 
            make_oil_type(field).c_str(), reg_num);
    root->reg_num = reg_num; reg_num++;
    if (ident->attributes.test(ATTR_vaddr)) {
        fprintf(oil_output, "&((*a%d)->f_%s)", ident->reg_num,
                field->lexinfo->c_str());
    } else {
        fprintf(oil_output, "&_");
        if (ident->blockNum > 0)
            fprintf(oil_output, "%d", ident->blockNum);
        fprintf(oil_output, "_%s", ident->lexinfo->c_str());
        fprintf(oil_output, "->f_%s",
                field->lexinfo->c_str());
    }
    fprintf(oil_output, ";\n");
  }
}

////////////////////////////////////////////////////////////////////////
