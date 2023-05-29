

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

void make_symap(std::vector<Symap>& vect, Symbol name, Symbol type) {
    Symap vt;
    vt.push_back(name);
    vt.push_back(type);
    vect.push_back(vt);
}

ostream& err_fun() {
    return global_table->semant_error(global_class);
}

Symbol lub_post(Symbol root, Symbol left, Symbol right) {
    if (graph.count(root) == 0) {
	if (root == left) {
	    return left;
        } else if (root == right) {
	    return right;
        } else {
	    return NULL;
        }
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
    } else if (num == 0) {
	return NULL;
    }
}

Symbol lub(Symbol left, Symbol right) {
    if (left == right) {
	return left;
    }
    
    if (left == SELF_TYPE) {
	left = global_class->get_name();
    }
    if (right == SELF_TYPE) {
	right = global_class->get_name();
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
   if (method_env.count(cl) == 0) 
      return null_method;
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
       for (int i = features->first(); features->more(i); i = features->next(i)) {
           features->nth(i)->traverse();
       }
       symtab->exitscope();
       return name;
}

Symbol formal_class::traverse() {
    Symbol f_name = name;

    if (type_decl == SELF_TYPE) {
	err_fun() << "Formal parameter " << name->get_string() << "cannot have type SELF_TYPE." << endl;
    }

    if (defined_classes.count(type_decl) == 0) {
	err_fun() << "Class " << type_decl->get_string() << " of formal parameter " << name->get_string() << " is undefined." << endl;
        symtab->addid(name, Object);
        return Object;
    } 

    if (symtab->probe(f_name) != NULL) {
        err_fun() << "Formal parameter " << f_name << " is multiply defined." << endl;
    } else {
        symtab->addid(f_name, type_decl); 
    }

    return type_decl;
}

Symbol method_class::traverse() {
    symtab->enterscope();
    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
        formals->nth(i)->traverse();
    }
   
    if (defined_classes.count(return_type) == 0) {
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
   Symbol sym = get_type_lookup(str);
   if (sym  == NULL) {
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
        if (len - 1 != vect.size()) {
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
	if (len - 1 != vect.size()) {
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

    for (int i = cases->first(); cases->more(i); i = cases->next(i)) {
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

void ClassTable::dealCycle(std::map<Symbol, Class_>& records,  int total) {
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
       return ;
    }

    for (std::map<Symbol, std::vector<Symbol> >::const_iterator it = graph.begin(); it != graph.end(); it++) {
        if (no_cyc.count(it->first) == 0) {
          this->semant_error(records[it->first]);
          this->error_stream << "Class " << it->first << ", or an ancestor of " << it->first << ", is involved in an inheritance cycle." << endl;
        }  
    }    
}


 
void traverse(Classes classes) {
    symtab->enterscope(); 
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        global_class = classes->nth(i);
        classes->nth(i)->traverse();
    }
    symtab->exitscope();
}

void method_class::gather(Symbol cl) {
    Symbol st = name;
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
    // check redefine
    obj_env[cl][st] = sym;
}

void class__class::gather(Symbol cl) {
    for (int i = features->first(); features->more(i); i = features->next(i)) {
        Feature ft = features->nth(i);
        ft->gather(cl);
    }
}

void gather_class_information(Classes classes) {
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        Class_ ct = classes->nth(i);
        Symbol cl = ct->pname;
        
        if (method_env.count(cl) == 0) {
            std::map<Symbol, std::vector<Symap> > m;
            method_env[cl] = m;
        }

        if (obj_env.count(cl) == 0) {
            std::map<Symbol, Symbol> m;
            m[self] = SELF_TYPE;
            obj_env[cl] = m;
        }

        ct->gather(cl);
/*                
        cout << cl << endl;
	cout << "method" << endl;
        std::map<Symbol, std::vector<Symap> >& t = method_env[cl];
        for (std::map<Symbol, std::vector<Symap> >::const_iterator it = t.begin(); it != t.end(); it++) {
            cout << it->first << " ";
            for (std::vector<Symap>::const_iterator itr = it->second.begin(); itr != it->second.end(); itr++ ) {
               cout << (*itr)[0]->get_string() << " " << (*itr)[1]->get_string() << endl;
            }
            cout << endl;
        }
        cout << "attr" << endl;
        std::map<Symbol, Symbol>& d = obj_env[cl];
        for (std::map<Symbol, Symbol>::const_iterator it = d.begin(); it != d.end(); it++) {
           std::cout << it->first << " " << it->second->get_string() << endl;
        }
*/
        /*ct->gather();
        std::string pt = ct->pparent->get_string();
        std::string st = ct->pname->get_string(); 

        Features features = ct->get_features();
        // gather attrs
        for (int j = features->first(); features->more(j); j = features->next(j)) {
            std::string fea_name = features->nth(j)->get_name()->get_string();
            bool is_method = features->nth(j)->is_method();
            if (is_method) {
                if (method_env[st].count(fea_name) == 1) {
                    semant_error(ct);
                    error_stream << "Method " << fea_name << " is multiply defined." << endl;
                } else {
                    std::vector<std::string> vt = new std::vector();
                    std::map<std::string, std::vector<std::string> >m = new std::map<std::string, std::vector<std::string> >();
                    m[fea_name] = vt;
                    method_env[st] = m;
                }
            } else {
                if (obj_env[pt].count(fea_name) == 1) {
                    semant_error(ct);
                    error_stream << "Attribute " << fea_name << " is multiply defined in class." << endl;
                } else {
                    std::map<std::string, std::string> m = new std::map<std::string, std::string>();
                    m[st] = 
                    obj_env[pt].insert();
                }
            }
        }
  */
    } 
}
 
ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {
    global_table = this;
    std::map<Symbol, Class_> records;
    std::map<Symbol, std::set<Symbol> > attr_mapper;
    std::map<Symbol, std::set<Symbol> > method_mapper;
    defined_classes.insert(Object);
    defined_classes.insert(IO);
    int num = 0;

    // deal with the undefined inherit
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        Symbol st = classes->nth(i)->pname;
        defined_classes.insert(st);
        records[st] = classes->nth(i);
        num++;
    }
    num += 5;
    // gather classes informations
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        int d = defined_classes.count(classes->nth(i)->pparent);
        Class_ ct = classes->nth(i);
        Symbol pt = ct->pparent;
        Symbol st = ct->pname;
         
        if (defined_classes.count(pt) == 0) {
            semant_error(classes->nth(i));
            if (st == Int || st == Str || st == Bool) {
                error_stream << "Class " << st << " cannot inherit class " << pt << "." << endl;
            } else {
                error_stream << "Class " << st->get_string() << " inherits from an undefined class " << pt->get_string() << "." << endl;
            }
        }
        // make the graph
        graph[pt].push_back(st);
        reverse_graph[st] = pt;
         
        Features features = ct->get_features();
        // gather attrs
        for (int j = features->first(); features->more(j); j = features->next(j)) {
            Symbol fea_name = features->nth(j)->get_name();
            bool is_method = features->nth(j)->is_method();
            if (is_method) {
                if (method_mapper[pt].count(fea_name) == 1) {
                    semant_error(ct);
                    error_stream << "Method " << fea_name->get_string() << " is multiply defined." << endl;       
                } else {
                    method_mapper[pt].insert(fea_name);
                }
            } else {
                if (attr_mapper[pt].count(fea_name) == 1) {
                    semant_error(ct);
                    error_stream << "Attribute " << fea_name->get_string() << " is multiply defined in class." << endl;
                } else {
                    attr_mapper[pt].insert(fea_name);
                }
            }
        }
    }

    // change these by Symbol
    graph[Object].push_back(Int);
    graph[Object].push_back(IO);
    graph[Object].push_back(Str);
    graph[Object].push_back(Bool);
    reverse_graph[Int] = Object;
    reverse_graph[IO] = Object;
    reverse_graph[Str] = Object;
    reverse_graph[Bool] = Object;

    dealCycle(records, num);

    defined_classes.insert(Bool);
    defined_classes.insert(Str);
    defined_classes.insert(Int);

    install_basic_classes();
    gather_class_information(classes);

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
    method_env[Object] = m_obj;
    method_env[IO] = m_io;
    method_env[Str] = m_str;

    std::vector<Symap> v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, vt;
    make_symap(v1, Object, Object);
    make_symap(v2, Str, Str);    
    make_symap(v3, SELF_TYPE, SELF_TYPE); 

    m_obj[cool_abort] = v1;
    m_obj[type_name] = v2;
    m_obj[copy] = v3;

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


