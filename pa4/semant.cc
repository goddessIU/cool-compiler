

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <string>
#include "semant.h"
#include "utilities.h"

extern int semant_debug;
extern char *curr_filename;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}

bool has_main_method = false;
bool has_main = false;
typedef std::vector<Symbol>  Symap;
static std::set<Symbol> defined_classes;
static std::map<Symbol, std::map<Symbol, Symbol> > obj_env;
static std::map<Symbol, std::map<Symbol, std::vector<Symap> > >method_env;
static std::map<Symbol, std::vector<Symbol> > graph;
static std::map<Symbol, Symbol> reverse_graph;
Class_ global_class;
ClassTable *global_table;
static SymbolTable<Symbol, Entry> *symtab = new SymbolTable<Symbol, Entry>();
static std::vector<Symap> null_method;
std::map<Symbol, Class_> records;
std::set<Symbol> bad_class;

bool has_type_with_self(Symbol s) {
    if (defined_classes.count(s) >= 1 || s == SELF_TYPE) {
	return true;
    }
    return false;
}

bool has_type(Symbol s) {
    if (defined_classes.count(s) >= 1) 
	return true;

    return false;
}

void make_symap(std::vector<Symap>& vect, Symbol name, Symbol type) {
    Symap vt;
    vt.push_back(name);
    vt.push_back(type);
    vect.push_back(vt);
}

ostream& err_fun() {
    return global_table->semant_error(global_class);
}

void test() {
    cout << "reverse : " << endl;
    for (std::map<Symbol, Symbol>::iterator it = reverse_graph.begin(); it != reverse_graph.end(); it++) {
	cout << it->first << ":" << it->second << endl;
    }

}

Symbol lub_post(Symbol root, Symbol left, Symbol right) {
    if (root == left || root == right) {
	return root;
    }

    std::vector<Symbol>& queue = graph[root];
    int num = 0;
    Symbol a = NULL;
    Symbol b = NULL;
    for (std::vector<Symbol>::const_iterator it = queue.begin(); it != queue.end(); it++) {
	Symbol m = lub_post(*it, left, right);
	if (m != NULL) {
	    if (num == 0) {
		num++;
 		a = m;
	    } else if (num == 1) {
		num++;
		b = m;
	    }
        }
    }
    if (num == 1) {
	return a;
    } else if (num == 2) {
	return root;
    } else {
	return NULL;
    }
}

Symbol lub(Symbol left, Symbol right) {
    if (left == SELF_TYPE && right == SELF_TYPE) {
	return SELF_TYPE;
    }

    if (left == SELF_TYPE) {
	left = global_class->get_name();
    }
    if (right == SELF_TYPE) {
	right = global_class->get_name();
    }

    if (left == right) {
        return left;
    }

    Symbol root = Object;
    return lub_post(root, left, right);
}

bool inher(Symbol parent, Symbol child) {
    if (parent == SELF_TYPE && child == SELF_TYPE) {
	return true;
    } else if (child == SELF_TYPE && parent != SELF_TYPE) {
	child = global_class->get_name();
    } else if (child != SELF_TYPE && parent == SELF_TYPE) {
	return false;
    }

    if (child == parent) {
	return true;
    }

    while (1) {
        if (reverse_graph.count(child) == 0) {
		return false;
        }
        if (reverse_graph[child] == parent) {
	    return true;
        } else {
 	    child = reverse_graph[child];
        }
    }
    return false;
}

Symbol get_type_probe(Symbol id) {
    if (symtab->probe(id) != NULL) {
        return symtab->probe(id);
    } else {
        return get_attr_type(id);
    }
}

Symbol get_type_lookup(Symbol id) {
    if (symtab->lookup(id) != NULL) {
        return symtab->lookup(id);
    } else {
        return get_attr_type(id);
    }
}

// O(v)
Symbol get_attr_type(Symbol attr) {
    Symbol class_name = global_class->get_name();
    while (1) {
       std::map<Symbol, Symbol>& mapper = obj_env[class_name];
       if (mapper.count(attr) == 1) {
           return mapper[attr];
       } else {
           if (reverse_graph.count(class_name) == 0) {
               return NULL;
           } else {
               class_name = reverse_graph[class_name];
           }   
       }
    }
}


// M(C, f)
std::vector<Symap>& get_method_type(Symbol func, Symbol cl) {
   while (1) {
       if (method_env[cl].count(func) == 1) 
              return method_env[cl][func];
       else {
          if (reverse_graph.count(cl) == 0) {
              return null_method;
          } else {
              cl = reverse_graph[cl];
          }
       }
   }
}

Symbol expression_traverse(Expression exp) {
    return exp->traverse();
}

void ClassTable::check_type(Symbol& sym) {
}

Symbol class__class::traverse(){
       symtab->enterscope();

       if (name == Main) {
	    if (get_method_type(main_meth, Main).size() == 0) {
		err_fun() << "No \'main\' method in class Main." << endl;
	    }
	}       
       for (int i = features->first(); features->more(i); i = features->next(i)) {
           features->nth(i)->traverse();
       }
       symtab->exitscope();
       return name;
}

Symbol formal_class::traverse() {
    Symbol f_name = name;

    if (type_decl == SELF_TYPE) {
	err_fun() << "Formal parameter " << name->get_string() << " cannot have type SELF_TYPE." << endl;
    }

    if (name == self) {
	err_fun() << "\'self\' cannot be the name of a formal parameter." << endl;
    }

    if (!has_type(type_decl)) {
	err_fun() << "Class " << type_decl->get_string() << " of formal parameter " << name->get_string() << " is undefined." << endl;
        return Object;
    } 

    if (symtab->probe(f_name) != NULL) {
        err_fun() << "Formal parameter " << f_name << " is multiply defined." << endl;
    } else {
        symtab->addid(f_name, type_decl); 
    }

    return type_decl;
}

bool redefined_check(Symbol name, std::vector<Symbol>& vect) {
    std::vector<Symap>& protos = get_method_type(name, reverse_graph[global_class->get_name()]); 
    int len1 = protos.size();
    int len2 = vect.size();
    if (len1 == 0) {
	return true;
    }

    if (protos[len1 - 1][1] != vect[len2 - 1]) {
	err_fun() << "In redefined method " << name->get_string() << ", return type " << vect[len2 - 1]->get_string() << "is different from original return type " << protos[len1 - 1][1]->get_string() << "." << endl;
	return false;
    } 

    if (protos.size() != vect.size()) {
	err_fun() << "Incompatible number of formal parameters in redefined method " << name->get_string() << "." << endl;
	return false;
    } 

    for (int i = 0; i < len2 - 1; i++) {
	if (vect[i] != protos[i][1]) {
	    err_fun() << "In redefined method " << name->get_string() << ", parameter type " << (vect[i])->get_string() << " is different from original type " << (protos[i][1])->get_string() << endl;
	    return false;
	}
    }  
    return true; 
}

Symbol method_class::traverse() {
    std::set<Symbol> bad_formal;
    std::vector<Symbol> formal_vect;

    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
	formal_vect.push_back(formals->nth(i)->get_ret_type());
    }
    formal_vect.push_back(return_type);
    redefined_check(name, formal_vect);
 
    symtab->enterscope();
    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
	Symbol name = formals->nth(i)->get_name();
	Symbol type = formals->nth(i)->get_ret_type();
	if (type == SELF_TYPE) {
	   bad_formal.insert(name);
           err_fun() << "Formal parameter " << name->get_string() << " cannot have type SELF_TYPE." << endl;
        }  

        if (name == self) {
            bad_formal.insert(name);
            err_fun() << "\'self\' cannot be the name of a formal parameter." << endl;
        } 
    }

    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
        Symbol name = formals->nth(i)->get_name();
        Symbol type = formals->nth(i)->get_ret_type();
	if (bad_formal.count(name) == 0) {
	     formals->nth(i)->traverse();
	}
    } 
   
    if (!has_type_with_self(return_type)) {
	err_fun() << "Undefined return type " << return_type->get_string() << " in method " << name->get_string() << "." << endl;
	expr->traverse();
        return Object;
    }

    Symbol t0 = expr->traverse();

    if (!inher(return_type, t0)) {
        err_fun() << "Inferrend return type " << t0->get_string() << " of method " << name->get_string() << " does not conform to declared return type " << return_type->get_string() << "." << endl;
    }

    symtab->exitscope();
    return return_type;
}

Symbol assign_class::traverse() {
   Symbol str = name;
   if (self == name) {
      err_fun() << "Cannot assign to \'self\'." << endl;
   }

   Symbol sym = get_type_lookup(str);
   if (sym  == NULL && name != self) {
       global_table->semant_error(global_class) << "Assignment to undeclared variable " << str << "." << endl;
       type = Object;
       return Object;
   } else {
       Symbol exp_type = expr->traverse();
       if (inher(sym, exp_type)) {
           type = exp_type;
           return exp_type;
       } else {
       	   err_fun() << "Type " << exp_type->get_string() << " of assigned expression does not conform to declared type " << sym->get_string() << " of identifier " << name->get_string() << "." << endl;
	   type = Object;
           return Object;
       }   
   }
}

Symbol static_dispatch_class::traverse() {
    if (type_name == SELF_TYPE) {
        err_fun() << "Static dispatch to SELF_TYPE." << endl;
	type = Object;
        return Object;  
    }

    if (defined_classes.count(type_name) == 0) {
       err_fun() << "Static dispatch to undefined class " << type_name->get_string() << "." << endl;
	type = Object;
       return Object;
    }

    Symbol t1;
    if (expr->is_noexpr()) {
        t1 = SELF_TYPE;
    } else {
        t1 = expression_traverse(expr);
    }
    // get the type of arguments
    std::vector<Symbol> vect;
    for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
        vect.push_back(expression_traverse(actual->nth(i)));
    }
   
    Symbol tp = type_name;
    if (!inher(tp, t1)) {
	err_fun() << "Expression type " << t1->get_string() << " does not conform to declared static dispatch type " << tp->get_string() << "." << endl;
    }

    std::vector<Symap>& method_proto = get_method_type(name, tp);
    if (method_proto.empty()) {
        err_fun() << "Static dispatch to undefined method " << name->get_string() << "." << endl;
        type =  Object;
        return type;
    } else {
        int len = method_proto.size();
        if (len - 1 != static_cast<int>(vect.size())) {
            err_fun() << "Method " << name->get_string() << " called with wrong number of arguments." << endl;
            type =  method_proto[len - 1][1];
 	    return type;
        } else {
            for (int i = 0; i < len - 1; i++) {
                if (!inher(method_proto[i][1], vect[i])) {
                    err_fun() << "In call of method " << name->get_string() << ", type " << vect[i] << " of parameter " << method_proto[i][0] << " does not conform to declared type " << method_proto[i][1] << "." << endl;
                }
            }

            Symbol t2 = method_proto[len-1][1];
            if (t2 == SELF_TYPE) {
	        type = t1;
                return t1;
            } else {
	        type = t2;
                return t2;
            }
        }
    }
}



Symbol dispatch_class::traverse() {
    // get the type of caller
    Symbol t1;
    if (expr->is_noexpr()) {
	t1 = SELF_TYPE;
    } else {
	t1 = expression_traverse(expr);
    }

    // get the type of arguments
    std::vector<Symbol> vect;
    for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
        vect.push_back(expression_traverse(actual->nth(i)));
    }    

    // get the real caller's type
    Symbol tp;
    if (t1 == SELF_TYPE) {
	tp = global_class->get_name();
    } else {
	tp = t1;
    }
    std::vector<Symap>& method_proto = get_method_type(name, tp);
    if (method_proto.empty()) {
	err_fun() << "Dispatch to undefined method " << name->get_string() << "." << endl;
	type = Object;
	return Object;
    } else {
        int len = method_proto.size();
	if (len - 1 != static_cast<int>(vect.size())) {
	    err_fun() << "Method " << name->get_string() << " called with wrong number of arguments." << endl;
	    type = method_proto[len - 1][1];
	    return method_proto[len - 1][1];
        } else {
            for (int i = 0; i < len - 1; i++) {
		if (!inher(method_proto[i][1], vect[i])) {
		    err_fun() << "In call of method " << name->get_string() << ", type " << vect[i] << " of parameter " << method_proto[i][0] << " does not conform to declared type " << method_proto[i][1] << "." << endl;
		}
            }
   
            Symbol t2 = method_proto[len-1][1];
            if (t2 == SELF_TYPE) {
		type = t1;
		return t1;
            } else {
		type = t2;
		return t2;
            }
        }
    }
}

Symbol cond_class::traverse() {
    Symbol t1 = expression_traverse(pred);
    Symbol t2 = expression_traverse(then_exp);
    Symbol t3 = expression_traverse(else_exp);
    if (t1 != Bool) {
	err_fun() << "Predicate of \'if\' does not have type Bool." << endl;
	type = Object;
	return Object;
    } else {
	Symbol t =  lub(t2, t3);
        if (t == NULL) {
	   type = Object;
	   return Object;
	}
	type = t;
	return t;
    }
}

Symbol loop_class::traverse() {
    Symbol t1 = expression_traverse(pred);
    if (t1 != Bool) {
        global_table->semant_error(global_class) << "Loop condition does not have type Bool." << endl;
    }
    Symbol t2 = expression_traverse(body);
    type = Object;
    return Object;
}

Symbol branch_class::traverse() {
    symtab->enterscope();

    if (defined_classes.count(type_decl) == 0) {
	err_fun() << "Class " << type_decl->get_string() << " of case branch is undefined." << endl;
        symtab->addid(name, Object);
        return Object;
    }

    symtab->addid(name, type_decl);
    Symbol t = expression_traverse(expr);
    symtab->exitscope();
    return t;
}

Symbol typcase_class::traverse() {
    Symbol t1 = expression_traverse(expr);
    Symbol last = NULL;
    bool flag = true;
    std::set<Symbol> used_type;    
    std::set<int> used_idx;

    for (int i = cases->first(); cases->more(i); i = cases->next(i)) {
	Symbol m = cases->nth(i)->get_type();
	if (used_type.count(m) == 0) {
	    used_type.insert(m);
	} else {
	    err_fun() << "Duplicate branch " << m->get_string() << " in case statement." << endl;
	    used_idx.insert(i);
	}
    }

    for (int i = cases->first(); cases->more(i); i = cases->next(i)) {
	if (used_idx.count(i) == 1) 
		continue;

        Symbol m = cases->nth(i)->traverse();
     
	if (flag) {
	   flag = false;
 	   last = m;
	} else {
	   last = lub(last, m);
	}
    }
	
    type = last;
    return last;
}


Symbol block_class::traverse() {
    symtab->enterscope();
    Symbol rt; 
    for (int i = body->first(); body->more(i); i = body->next(i)) {
        rt = body->nth(i)->traverse();
    }
    symtab->exitscope();
    type = rt;
    return rt;
}



Symbol let_class::traverse() {
    symtab->enterscope();
    Symbol td = type_decl;
    if (identifier == self) {
	err_fun() << "\'self\' cannot be bound in a \'let\' expression." << endl;
	symtab->addid(identifier, SELF_TYPE);
        Symbol ret = expression_traverse(body);
        symtab->exitscope();
        type = ret;
        return ret;
    }

    if (defined_classes.count(type_decl) == 0 && type_decl != SELF_TYPE) {
	err_fun() << "Class " << type_decl->get_string() << " of let-bound identifier " << identifier->get_string() << " is undefined." << endl;
	td = Object;
    } 

    if (init->is_noexpr()) {
       symtab->addid(identifier, td);
    } else {
       Symbol t1 = expression_traverse(init);
       if (inher(td, t1)) {
           symtab->addid(identifier, td);
       } else {
	   err_fun() << "Inferred type " << t1->get_string() << " of initialization of " << identifier->get_string() << " does not conform to identifier\'s declared type " << type_decl->get_string() << "." << endl;
	   symtab->addid(identifier, td);
       } 
     }
  
    Symbol ret = expression_traverse(body);
    symtab->exitscope();
    type = ret;
    return ret;
}


Symbol plus_class::traverse() {
    Symbol t1 = expression_traverse(e1);
    Symbol t2 = expression_traverse(e2);
    if (t1 == Int && t2 == Int) {
	type = Int;
	return Int;
    } else {
        err_fun() << "non-Int arguments: " << t1->get_string() << " + " << t2->get_string() << endl;	
	type = Object;
	return Object;
    }
}


Symbol sub_class::traverse() {
    Symbol t1 = expression_traverse(e1);
    Symbol t2 = expression_traverse(e2);
    if (t1 == Int && t2 == Int) {
	type = Int;
        return Int;
    } else {
        err_fun() << "non-Int arguments: " << t1->get_string() << " - " << t2->get_string() << endl;
	type = Object;
        return Object;
    }
}

Symbol mul_class::traverse() {
    Symbol t1 = expression_traverse(e1);
    Symbol t2 = expression_traverse(e2);
    if (t1 == Int && t2 == Int) {
	type = Int;
        return Int;
    } else {
        err_fun() << "non-Int arguments: " << t1->get_string() << " * " << t2->get_string() << endl;
	type = Object;
        return Object;
    }
}


Symbol divide_class::traverse() {
    Symbol t1 = expression_traverse(e1);
    Symbol t2 = expression_traverse(e2);
    if (t1 == Int && t2 == Int) {
	type = Int;
        return Int;
    } else {
        err_fun() << "non-Int arguments: " << t1->get_string() << " / " << t2->get_string() << endl;
	type = Object;
        return Object;
    }
}

Symbol neg_class::traverse() {
    Symbol t = expression_traverse(e1);
    if (t == Int) {
	type = Int;
	return Int;
    } else {
        global_table->semant_error(global_class) << "Argument of \'~\' has type " << t->get_string() << " instead of Int." << endl; 	
	type = Object;
	return Object;
    }
}


Symbol lt_class::traverse() {
    Symbol t1 = expression_traverse(e1);
    Symbol t2 = expression_traverse(e2);
    if (t1 != Int || t2 != Int) {
        global_table->semant_error(global_class) << "non-Int arguments: " << t1->get_string() << " < " << t2->get_string() << endl;
	type = Object;
        return Object;
    };
    type = Bool;
    return Bool;
}



Symbol eq_class::traverse() {
    Symbol t1 = expression_traverse(e1);
    Symbol t2 = expression_traverse(e2);
    std::set<Symbol> types;
    types.insert(Int);
    types.insert(Bool);
    types.insert(Str);
    int n1 = types.count(t1);
    int n2 = types.count(t2);
    if (n1 == 0 && n2 == 0) {
	type = Bool;
	return Bool;
    } else if (n1 == 1 && n2 == 1) {
	if (t1 == t2) {
	   type = Bool;
	   return Bool;
	} else {
           err_fun() << "Illegal comparison with a basic type." << endl;
	   type = Object;
	   return Object;
	}
    } else {
        err_fun() << "Illegal comparison with a basic type." << endl;
	type = Object;
	return Object;	
    }
}


Symbol leq_class::traverse() {
    Symbol t1 = expression_traverse(e1);
    Symbol t2 = expression_traverse(e2);
    if (t1 != Int || t2 != Int) {
        global_table->semant_error(global_class) << "non-Int arguments: " << t1->get_string() << " <= " << t2->get_string() << endl;
	type = Object;
	return Object;
    };
    type = Bool;
    return Bool;
}


Symbol comp_class::traverse() {
    Symbol t = expression_traverse(e1);
    if (t == Bool) {
	type = Bool;
	return Bool;
    } else {
        global_table->semant_error(global_class) << "Argument of \'not\' has type " << t->get_string() << " instead of Bool." << endl;
	type = Object;
	return Object;
    }
}



Symbol int_const_class::traverse() {
    type = Int;
    return Int;
}


Symbol bool_const_class::traverse() {
    type = Bool;
    return Bool;
}


Symbol string_const_class::traverse() {
    type = Str;
    return Str;
}




Symbol new__class::traverse() {
   if (defined_classes.count(type_name) == 0 && type_name != SELF_TYPE) {
	err_fun() << "\'new\' used with undefined class " << type_name->get_string() << "." << endl;
	type = Object;
        return Object;
   }

   if (type_name == SELF_TYPE) {
	type = SELF_TYPE;
	return SELF_TYPE;
   } else {
	type = type_name;
	return type_name;
   } 
}

Symbol isvoid_class::traverse() {
    expression_traverse(e1);
    type = Bool;
    return Bool;
}

Symbol no_expr_class::traverse() {
    type = No_type;
    return No_type;
}

Symbol object_class::traverse() {
    if (get_type_lookup(name) == NULL) {
	global_table->semant_error(global_class) << "Undeclared identifier " << name->get_string() << "." << endl;
	type = Object;
    	return Object;
    }
    type = get_type_lookup(name);
    return get_type_lookup(name);
}

Symbol attr_class::traverse() {
    if (defined_classes.count(type_decl) == 0 && type_decl != SELF_TYPE) {
	err_fun() << "Class " << global_class->get_name()->get_string() << " of attribute " << name->get_string() << " is undefined." << endl;
    }
    if (init->is_noexpr()) {
	return type_decl;
    } else {
	Symbol t1 = init->traverse();
        if (inher(type_decl, t1)) {
	    return type_decl;
        } else {
            err_fun() << "Inferred type " << t1->get_string() << " of initialization of attribute " << name->get_string() << " does not conform to declared type " << type_decl->get_string() << "." << endl;	    
 	    return type_decl;
	}
    }
}

bool ClassTable::dealCycle(int total) {
    std::set<Symbol> no_cyc;
    int num = 0;
    std::queue<Symbol> complete_queue;
    Symbol st = Object;
    complete_queue.push(st);
    while (complete_queue.size() != 0) {
        st = complete_queue.front();
        no_cyc.insert(st);
        complete_queue.pop();
        std::vector<Symbol>& childrens = graph[st];
        num++;
        for (std::vector<Symbol>::const_iterator chit = childrens.begin(); chit != childrens.end(); chit++) {
            complete_queue.push(*chit);
        }
    }
    
    if (num == total) {
       return false;
    }

    for (std::map<Symbol, std::vector<Symbol> >::const_iterator it = graph.begin(); it != graph.end(); it++) {
        if (no_cyc.count(it->first) == 0) {
          this->semant_error(records[it->first]);
          this->error_stream << "Class " << it->first << ", or an ancestor of " << it->first << ", is involved in an inheritance cycle." << endl;
        }  
    }    
    return true;
}


 
void traverse(Classes classes) {
    symtab->enterscope(); 
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
	if (bad_class.count(classes->nth(i)->get_name()) >= 1) 
		continue;

        global_class = classes->nth(i);
        classes->nth(i)->traverse();
    }
    symtab->exitscope();
}

void method_class::gather(Symbol cl) {
    Symbol st = name;

    if (st == main_meth) {
	has_main = true;
    }

    if (method_env[cl].count(st) == 0) {
        std::vector<Symap> vt;
        method_env[cl][st] = vt;
    } 
    
    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
        Symbol name = formals->nth(i)->get_name();
        Symbol ret_type = formals->nth(i)->get_ret_type();
        Symap s;
        s.push_back(name);
        s.push_back(ret_type);
        method_env[cl][st].push_back(s); 
    }

    //check refdine
    Symbol sym = return_type; 
    Symap s;
    s.push_back(sym);
    s.push_back(sym);
    method_env[cl][st].push_back(s);
}

void attr_class::gather(Symbol cl) {
    Symbol st = name;
    Symbol sym = type_decl;
    obj_env[cl][st] = sym;
}

bool is_inher_attr(Symbol attr, Symbol cl) {
    Symbol pt;
    while (reverse_graph.count(cl) != 0) {
	pt = reverse_graph[cl];
        if (obj_env[pt].count(attr) >= 1) {
	    return true;
	} 
	cl = pt;
    }
   
    return false;
}

void class__class::gather(Symbol cl) {
    std::set<Symbol> inher_attr;
    std::set<Symbol> has_attr;
    std::set<Symbol> has_method;
    
    for (int i = features->first(); features->more(i); i = features->next(i)) {
        Feature ft = features->nth(i);
        if (!ft->is_method() && ft->get_name() != self) {
            ft->gather(cl);
        }
    }
    
    for (int i = features->first(); features->more(i); i = features->next(i)) {
	Feature ft = features->nth(i);
	if (!ft->is_method()) {
	    if (is_inher_attr(ft->get_name(), cl)) {
		inher_attr.insert(ft->get_name());	
            }
	}
    }

    for (int i = features->first(); features->more(i); i = features->next(i)) {
	Feature ft = features->nth(i);
	if (ft->is_method()) {
	    if (has_method.count(ft->get_name()) >= 1) {
		err_fun() << "Method " << ft->get_name()->get_string() << " is multiply defined." << endl;
	    } else {
		has_method.insert(ft->get_name());
		ft->gather(cl);
	    }
	} else {
	    if (ft->get_name() == self) {
		err_fun() << "\'self\' cannot be the name of an attribute." << endl;
	    } else if (inher_attr.count(ft->get_name()) >= 1) {
         	err_fun() << "Attribute " << ft->get_name()->get_string() << " is an attribute of an inherited class." << endl;
		obj_env[cl].erase(ft->get_name());
 	    } else {
		if (has_attr.count(ft->get_name()) >= 1) {
		    err_fun() << "Attribute " << ft->get_name()->get_string() << " is multiply defined in class." << endl;
		} else {
		    has_attr.insert(ft->get_name());
		    ft->gather(cl);
		}	
            }
	}
    }
}

void gather_class_information(Classes classes) {
    std::vector<Class_> classes_queue;
    std::queue<Symbol> completed;
    completed.push(Object);  
    while (completed.size() > 0) {
	Symbol s = completed.front();
	completed.pop();
	if (s != Object && s != Int && s != Str && s != Bool && s!= IO && s != SELF_TYPE) {
	    classes_queue.push_back(records[s]);
	}
	std::vector<Symbol>& vect = graph[s];
        for (std::vector<Symbol>::const_iterator it = vect.begin(); it != vect.end(); it++) {
	    completed.push(*it);
	}
    } 
 
    for (std::vector<Class_>::const_iterator it = classes_queue.begin(); it != classes_queue.end(); it++) {
	Class_ cl = *it;
	Symbol s = cl->get_name();
	if (bad_class.count(s) >= 1) {
            continue;
        }

        if (s != Object && s != Int && s != Str && s != Bool && s != IO) {
            if (method_env.count(s) == 0) {
                std::map<Symbol, std::vector<Symap> > m;
                method_env[s] = m;
            }

            if (obj_env.count(s) == 0) {
                std::map<Symbol, Symbol> m;
                m[self] = SELF_TYPE;
                obj_env[s] = m;
            }
	}

	cl->gather(s);
    }
/*
    for (std::map<Symbol, std::vector<Symbol> >::const_iterator it = graph.begin(); it != graph.end(); it++) {
	cout << "parent: " << it->first << endl;
	for (std::vector<Symbol>::const_iterator itr = it->second.begin(); itr != it->second.end(); itr++) {
	   cout << "    " << *itr;
        }
        cout << endl;
    }

  */ 
  /* 
    for (std::map<Symbol, std::map<Symbol, std::vector<Symap> > >::const_iterator it = method_env.begin(); it != method_env.end(); it++) {
	cout << it->first << endl;
	for (std::map<Symbol, std::vector<Symap> >::const_iterator itr = it->second.begin(); itr != it->second.end(); itr++) {
	    cout << "  method name: " << itr->first << endl;
	    cout << itr->second.size() << endl;
	}
    }
*/ 
}

//make defined_classes
// redined class, inherits from undefined class
bool first_classes(Classes classes) {
   bool flag = false;
   std::set<Symbol> checked; 

   defined_classes.insert(Object);
   defined_classes.insert(IO);
   checked.insert(Object);
   checked.insert(IO);

   for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        Symbol st = classes->nth(i)->get_name();
	global_class = classes->nth(i);

	if (st == Main) {
	    has_main_method = true;
	}

        if (st == Int || st == IO || st == Object || st == Str || st == Bool || st == SELF_TYPE) {
	   err_fun() << "Redefinition of basic class " << st->get_string() << "." << endl;
	   bad_class.insert(st);
           flag = true;
	   continue;
	} 

        if (checked.count(st) == 1) {
	    err_fun() << "Class " << st->get_string() << " was previously defined." << endl;
            flag = true;
            bad_class.insert(st);
        } else {
  	    checked.insert(st);
        }
    }

    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
	Class_ ct = classes->nth(i);
	Symbol pt = ct->get_parent();
        Symbol st = ct->get_name();
	global_class = ct;

	if (bad_class.count(st) >= 1) {
	    continue;
	}

        if (checked.count(pt) == 0) {
            if (pt == Int || pt == Str || pt == Bool || pt == SELF_TYPE) {
                err_fun() << "Class " << st->get_string() << " cannot inherit class " << pt->get_string() << "." << endl;
	        bad_class.insert(st);
		flag = true;
            }
        }
    }

    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        Class_ ct = classes->nth(i);
        Symbol pt = ct->get_parent();
        Symbol st = ct->get_name();
        global_class = ct;

        if (bad_class.count(st) >= 1) {
            continue;
        }
        
        if (checked.count(pt) == 0) {
            if (pt != Int && pt != Str && pt != Bool) {
                err_fun() << "Class " << st->get_string() << " inherits from an undefined class " << pt->get_string() << "." << endl;
		bad_class.insert(st);
		flag = true;
            }
        } else {
	    defined_classes.insert(st);
        }
    }
 
    defined_classes.insert(Int);
    defined_classes.insert(Str);
    defined_classes.insert(Bool);

    return flag;
}
 
void construct_env(Classes classes) {
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        Class_ ct = classes->nth(i);
        Symbol pt = ct->get_parent();
        Symbol st = ct->get_name();

	if (bad_class.count(st) >= 1) {
	   continue;
	}

        global_class = ct;
	graph[pt].push_back(st);
        reverse_graph[st] = pt;
        records[st] = ct;
    }

    graph[Object].push_back(Int);
    graph[Object].push_back(IO);
    graph[Object].push_back(Str);
    graph[Object].push_back(Bool);

    reverse_graph[Int] = Object;
    reverse_graph[IO] = Object;
    reverse_graph[Str] = Object;
    reverse_graph[Bool] = Object;
}

bool check_main(Classes classes) {
    bool flag = false;

    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
	if (classes->nth(i)->get_name() == Main) {
	    flag = true;
	    break;
	}
    }

    if (!flag) {
	global_table->semant_error() << "Class Main is not defined." << endl;
    } 

    return flag;
}


ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {
    global_table = this;
    check_main(classes);

    first_classes(classes);
    dealCycle(classes->len() + 5);

    construct_env(classes);
    //maybe need more 
    install_basic_classes();
    gather_class_information(classes);

 
   //test();
    traverse(classes);
}

void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    std::map<Symbol, std::vector<Symap> >m_obj,  m_io, m_str;

    std::vector<Symap> v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, vt;
    make_symap(v1, Object, Object);
    make_symap(v2, Str, Str);    
    make_symap(v3, SELF_TYPE, SELF_TYPE); 

    m_obj[cool_abort] = v1;
    m_obj[type_name] = v2;
    m_obj[copy] = v3;
    method_env[Object] = m_obj;

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    make_symap(v4, Str, Str);
    make_symap(v4, SELF_TYPE, SELF_TYPE);
    make_symap(v5, Int, Int);
    make_symap(v5, SELF_TYPE, SELF_TYPE);
    make_symap(v6, Str, Str);
    make_symap(v7, Int, Int);
    m_io[out_string] = v4;
    m_io[out_int] = v5;
    m_io[in_string] = v6;
    m_io[in_int] = v7;
    method_env[IO] = m_io;
  
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);  

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    make_symap(v8, Int, Int);
    make_symap(v9, arg, Str);
    make_symap(v9, Str, Str);
    make_symap(v10, arg, Int);
    make_symap(v10, arg2, Int);
    make_symap(v10, Str, Str);  
    m_str[length] = v8;
    m_str[concat] = v9;
    m_str[substr] = v10;
    method_env[Str] = m_str;

    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 



/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();

    /* ClassTable constructor may do some semantic analysis */
    ClassTable *classtable = new ClassTable(classes);

    /* some semantic analysis code may go here */

    if (classtable->errors()) {
	cerr << "Compilation halted due to static semantic errors." << endl;
	exit(1);
    }
}


