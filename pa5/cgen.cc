
//**************************************************************
//
// Code generator SKELETON
//
// Read the comments carefully. Make sure to
//    initialize the base class tags in
//       `CgenClassTable::CgenClassTable'
//
//    Add the label for the dispatch tables to
//       `IntEntry::code_def'
//       `StringEntry::code_def'
//       `BoolConst::code_def'
//
//    Add code to emit everyting else that is needed
//       in `CgenClassTable::code'
//
//
// The files as provided will produce code to begin the code
// segments, declare globals, and emit constants.  You must
// fill in the rest.
//
//**************************************************************

#include "cgen.h"
#include "cgen_gc.h"
#include <map>

extern void emit_string_constant(ostream& str, char *s);
extern int cgen_debug;
int label_num = 0;
idx_map_type disp_map;
idx_map_type attr_map; 
Symbol so;
std::map<Symbol, int> tag_map;
std::map<Symbol, CgenNodeP> sym_node_map;

struct offset {
	int pos;
	// type == 0, formal  1  local, 2 label
	int type;
	offset(int a, int b) {
	type = a;
	pos = b;
	}
};
SymbolTable<Symbol, struct offset> local_scope;
int local_num = 1;

int get_attr_idx(Symbol type, Symbol name);
//
// Three symbols from the semantic analyzer (semant.cc) are used.
// If e : No_type, then no code is generated for e.
// Special code is generated for new SELF_TYPE.
// The name "self" also generates code different from other references.
//
//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
Symbol 
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

static char *gc_init_names[] =
  { "_NoGC_Init", "_GenGC_Init", "_ScnGC_Init" };
static char *gc_collect_names[] =
  { "_NoGC_Collect", "_GenGC_Collect", "_ScnGC_Collect" };


//  BoolConst is a class that implements code generation for operations
//  on the two booleans, which are given global names here.
BoolConst falsebool(FALSE);
BoolConst truebool(TRUE);

//*********************************************************
//
// Define method for code generation
//
// This is the method called by the compiler driver
// `cgtest.cc'. cgen takes an `ostream' to which the assembly will be
// emmitted, and it passes this and the class list of the
// code generator tree to the constructor for `CgenClassTable'.
// That constructor performs all of the work of the code
// generator.
//
//*********************************************************

void program_class::cgen(ostream &os) 
{
  // spim wants comments to start with '#'
  os << "# start of generated code\n";

  initialize_constants();
  CgenClassTable *codegen_classtable = new CgenClassTable(classes,os);

  os << "\n# end of generated code\n";
}


//////////////////////////////////////////////////////////////////////////////
//
//  emit_* procedures
//
//  emit_X  writes code for operation "X" to the output stream.
//  There is an emit_X for each opcode X, as well as emit_ functions
//  for generating names according to the naming conventions (see emit.h)
//  and calls to support functions defined in the trap handler.
//
//  Register names and addresses are passed as strings.  See `emit.h'
//  for symbolic names you can use to refer to the strings.
//
//////////////////////////////////////////////////////////////////////////////

static void emit_load(char *dest_reg, int offset, char *source_reg, ostream& s)
{
  s << LW << dest_reg << " " << offset * WORD_SIZE << "(" << source_reg << ")" 
    << endl;
}

static void emit_store(char *source_reg, int offset, char *dest_reg, ostream& s)
{
  s << SW << source_reg << " " << offset * WORD_SIZE << "(" << dest_reg << ")"
      << endl;
}

static void emit_load_imm(char *dest_reg, int val, ostream& s)
{ s << LI << dest_reg << " " << val << endl; }

static void emit_load_address(char *dest_reg, char *address, ostream& s)
{ s << LA << dest_reg << " " << address << endl; }

static void emit_partial_load_address(char *dest_reg, ostream& s)
{ s << LA << dest_reg << " "; }

static void emit_load_bool(char *dest, const BoolConst& b, ostream& s)
{
  emit_partial_load_address(dest,s);
  b.code_ref(s);
  s << endl;
}

static void emit_load_string(char *dest, StringEntry *str, ostream& s)
{
  emit_partial_load_address(dest,s);
  str->code_ref(s);
  s << endl;
}

static void emit_load_int(char *dest, IntEntry *i, ostream& s)
{
  emit_partial_load_address(dest,s);
  i->code_ref(s);
  s << endl;
}

static void emit_move(char *dest_reg, char *source_reg, ostream& s)
{ s << MOVE << dest_reg << " " << source_reg << endl; }

static void emit_neg(char *dest, char *src1, ostream& s)
{ s << NEG << dest << " " << src1 << endl; }

static void emit_add(char *dest, char *src1, char *src2, ostream& s)
{ s << ADD << dest << " " << src1 << " " << src2 << endl; }

static void emit_addu(char *dest, char *src1, char *src2, ostream& s)
{ s << ADDU << dest << " " << src1 << " " << src2 << endl; }

static void emit_addiu(char *dest, char *src1, int imm, ostream& s)
{ s << ADDIU << dest << " " << src1 << " " << imm << endl; }

static void emit_div(char *dest, char *src1, char *src2, ostream& s)
{ s << DIV << dest << " " << src1 << " " << src2 << endl; }

static void emit_mul(char *dest, char *src1, char *src2, ostream& s)
{ s << MUL << dest << " " << src1 << " " << src2 << endl; }

static void emit_sub(char *dest, char *src1, char *src2, ostream& s)
{ s << SUB << dest << " " << src1 << " " << src2 << endl; }

static void emit_sll(char *dest, char *src1, int num, ostream& s)
{ s << SLL << dest << " " << src1 << " " << num << endl; }

static void emit_jalr(char *dest, ostream& s)
{ s << JALR << "\t" << dest << endl; }

static void emit_jal(char *address,ostream &s)
{ s << JAL << address << endl; }

static void emit_return(ostream& s)
{ s << RET << endl; }

static void emit_gc_assign(ostream& s)
{ s << JAL << "_GenGC_Assign" << endl; }

static void emit_disptable_ref(Symbol sym, ostream& s)
{  s << sym << DISPTAB_SUFFIX; }

static void emit_init_ref(Symbol sym, ostream& s)
{ s << sym << CLASSINIT_SUFFIX; }

static void emit_label_ref(int l, ostream &s)
{ s << "label" << l; }

static void emit_protobj_ref(Symbol sym, ostream& s)
{ s << sym << PROTOBJ_SUFFIX; }

static void emit_method_ref(Symbol classname, Symbol methodname, ostream& s)
{ s << classname << METHOD_SEP << methodname; }

static void emit_label_def(int l, ostream &s)
{
  emit_label_ref(l,s);
  s << ":" << endl;
}

static void emit_beqz(char *source, int label, ostream &s)
{
  s << BEQZ << source << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_beq(char *src1, char *src2, int label, ostream &s)
{
  s << BEQ << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bne(char *src1, char *src2, int label, ostream &s)
{
  s << BNE << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bleq(char *src1, char *src2, int label, ostream &s)
{
  s << BLEQ << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_blt(char *src1, char *src2, int label, ostream &s)
{
  s << BLT << src1 << " " << src2 << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_blti(char *src1, int imm, int label, ostream &s)
{
  s << BLT << src1 << " " << imm << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_bgti(char *src1, int imm, int label, ostream &s)
{
  s << BGT << src1 << " " << imm << " ";
  emit_label_ref(label,s);
  s << endl;
}

static void emit_branch(int l, ostream& s)
{
  s << BRANCH;
  emit_label_ref(l,s);
  s << endl;
}

//
// Push a register on the stack. The stack grows towards smaller addresses.
//
static void emit_push(char *reg, ostream& str)
{
  emit_store(reg,0,SP,str);
  emit_addiu(SP,SP,-4,str);
}

//
// Fetch the integer value in an Int object.
// Emits code to fetch the integer value of the Integer object pointed
// to by register source into the register dest
//
static void emit_fetch_int(char *dest, char *source, ostream& s)
{ emit_load(dest, DEFAULT_OBJFIELDS, source, s); }

//
// Emits code to store the integer value contained in register source
// into the Integer object pointed to by dest.
//
static void emit_store_int(char *source, char *dest, ostream& s)
{ emit_store(source, DEFAULT_OBJFIELDS, dest, s); }


static void emit_test_collector(ostream &s)
{
  emit_push(ACC, s);
  emit_move(ACC, SP, s); // stack end
  emit_move(A1, ZERO, s); // allocate nothing
  s << JAL << gc_collect_names[cgen_Memmgr] << endl;
  emit_addiu(SP,SP,4,s);
  emit_load(ACC,0,SP,s);
}

static void emit_gc_check(char *source, ostream &s)
{
  if (source != (char*)A1) emit_move(A1, source, s);
  s << JAL << "_gc_check" << endl;
}


///////////////////////////////////////////////////////////////////////////////
//
// coding strings, ints, and booleans
//
// Cool has three kinds of constants: strings, ints, and booleans.
// This section defines code generation for each type.
//
// All string constants are listed in the global "stringtable" and have
// type StringEntry.  StringEntry methods are defined both for String
// constant definitions and references.
//
// All integer constants are listed in the global "inttable" and have
// type IntEntry.  IntEntry methods are defined for Int
// constant definitions and references.
//
// Since there are only two Bool values, there is no need for a table.
// The two booleans are represented by instances of the class BoolConst,
// which defines the definition and reference methods for Bools.
//
///////////////////////////////////////////////////////////////////////////////

//
// Strings
//
void StringEntry::code_ref(ostream& s)
{
  s << STRCONST_PREFIX << index;
}

//
// Emit code for a constant String.
// You should fill in the code naming the dispatch table.
//

void StringEntry::code_def(ostream& s, int stringclasstag)
{
  IntEntryP lensym = inttable.add_int(len);

  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s  << LABEL                                             // label
      << WORD << stringclasstag << endl                                 // tag
      << WORD << (DEFAULT_OBJFIELDS + STRING_SLOTS + (len+4)/4) << endl // size
      << WORD;


 /***** Add dispatch information for class String ******/
      s << STRINGNAME << DISPTAB_SUFFIX; 
      s << endl;                                              // dispatch table
      s << WORD;  lensym->code_ref(s);  s << endl;            // string length
  emit_string_constant(s,str);                                // ascii string
  s << ALIGN;                                                 // align to word
}

//
// StrTable::code_string
// Generate a string object definition for every string constant in the 
// stringtable.
//
void StrTable::code_string_table(ostream& s, int stringclasstag)
{  
  for (List<StringEntry> *l = tbl; l; l = l->tl())
    l->hd()->code_def(s,stringclasstag);
}

//
// Ints
//
void IntEntry::code_ref(ostream &s)
{
  s << INTCONST_PREFIX << index;
}

//
// Emit code for a constant Integer.
// You should fill in the code naming the dispatch table.
//

void IntEntry::code_def(ostream &s, int intclasstag)
{
  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s << LABEL                                // label
      << WORD << intclasstag << endl                      // class tag
      << WORD << (DEFAULT_OBJFIELDS + INT_SLOTS) << endl  // object size
      << WORD; 

 /***** Add dispatch information for class Int ******/
      s << INTNAME << DISPTAB_SUFFIX;
      s << endl;                                          // dispatch table
      s << WORD << str << endl;                           // integer value
}


//
// IntTable::code_string_table
// Generate an Int object definition for every Int constant in the
// inttable.
//
void IntTable::code_string_table(ostream &s, int intclasstag)
{
  for (List<IntEntry> *l = tbl; l; l = l->tl())
    l->hd()->code_def(s,intclasstag);
}


//
// Bools
//
BoolConst::BoolConst(int i) : val(i) { assert(i == 0 || i == 1); }

void BoolConst::code_ref(ostream& s) const
{
  s << BOOLCONST_PREFIX << val;
}
  
//
// Emit code for a constant Bool.
// You should fill in the code naming the dispatch table.
//

void BoolConst::code_def(ostream& s, int boolclasstag)
{
  // Add -1 eye catcher
  s << WORD << "-1" << endl;

  code_ref(s);  s << LABEL                                  // label
      << WORD << boolclasstag << endl                       // class tag
      << WORD << (DEFAULT_OBJFIELDS + BOOL_SLOTS) << endl   // object size
      << WORD;

 /***** Add dispatch information for class Bool ******/
      s << BOOLNAME << DISPTAB_SUFFIX;
      s << endl;                                            // dispatch table
      s << WORD << val << endl;                             // value (0 or 1)
}

//////////////////////////////////////////////////////////////////////////////
//
//  CgenClassTable methods
//
//////////////////////////////////////////////////////////////////////////////

//***************************************************
//
//  Emit code to start the .data segment and to
//  declare the global names.
//
//***************************************************

void CgenClassTable::code_global_data()
{
  Symbol main    = idtable.lookup_string(MAINNAME);
  Symbol string  = idtable.lookup_string(STRINGNAME);
  Symbol integer = idtable.lookup_string(INTNAME);
  Symbol boolc   = idtable.lookup_string(BOOLNAME);

  str << "\t.data\n" << ALIGN;
  //
  // The following global names must be defined first.
  //
  str << GLOBAL << CLASSNAMETAB << endl;
  str << GLOBAL; emit_protobj_ref(main,str);    str << endl;
  str << GLOBAL; emit_protobj_ref(integer,str); str << endl;
  str << GLOBAL; emit_protobj_ref(string,str);  str << endl;
  str << GLOBAL; falsebool.code_ref(str);  str << endl;
  str << GLOBAL; truebool.code_ref(str);   str << endl;
  str << GLOBAL << INTTAG << endl;
  str << GLOBAL << BOOLTAG << endl;
  str << GLOBAL << STRINGTAG << endl;

  //
  // We also need to know the tag of the Int, String, and Bool classes
  // during code generation.
  //
  str << INTTAG << LABEL
      << WORD << intclasstag << endl;
  str << BOOLTAG << LABEL 
      << WORD << boolclasstag << endl;
  str << STRINGTAG << LABEL 
      << WORD << stringclasstag << endl;    
}


//***************************************************
//
//  Emit code to start the .text segment and to
//  declare the global names.
//
//***************************************************

void CgenClassTable::code_global_text()
{
  str << GLOBAL << HEAP_START << endl
      << HEAP_START << LABEL 
      << WORD << 0 << endl
      << "\t.text" << endl
      << GLOBAL;
  emit_init_ref(idtable.add_string("Main"), str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("Int"),str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("String"),str);
  str << endl << GLOBAL;
  emit_init_ref(idtable.add_string("Bool"),str);
  str << endl << GLOBAL;
  emit_method_ref(idtable.add_string("Main"), idtable.add_string("main"), str);
  str << endl;
}

void CgenClassTable::code_bools(int boolclasstag)
{
  falsebool.code_def(str,boolclasstag);
  truebool.code_def(str,boolclasstag);
}

void CgenClassTable::code_select_gc()
{
  //
  // Generate GC choice constants (pointers to GC functions)
  //
//  cgen_Memmgr = GC_GENGC;
//  cgen_Memmgr_Test = GC_TEST;
  str << GLOBAL << "_MemMgr_INITIALIZER" << endl;
  str << "_MemMgr_INITIALIZER:" << endl;
  str << WORD << gc_init_names[cgen_Memmgr] << endl;
  str << GLOBAL << "_MemMgr_COLLECTOR" << endl;
  str << "_MemMgr_COLLECTOR:" << endl;
  str << WORD << gc_collect_names[cgen_Memmgr] << endl;
  str << GLOBAL << "_MemMgr_TEST" << endl;
  str << "_MemMgr_TEST:" << endl;
  str << WORD << (cgen_Memmgr_Test == GC_TEST) << endl;
}


//********************************************************
//
// Emit code to reserve space for and initialize all of
// the constants.  Class names should have been added to
// the string table (in the supplied code, is is done
// during the construction of the inheritance graph), and
// code for emitting string constants as a side effect adds
// the string's length to the integer table.  The constants
// are emmitted by running through the stringtable and inttable
// and producing code for each entry.
//
//********************************************************

void CgenClassTable::code_constants()
{
  //
  // Add constants that are required by the code generator.
  //
  stringtable.add_string("");
  inttable.add_string("0");

  stringtable.code_string_table(str,stringclasstag);
  inttable.code_string_table(str,intclasstag);
  code_bools(boolclasstag);
}


CgenClassTable::CgenClassTable(Classes classes, ostream& s) : nds(NULL) , str(s)
{
   tag_counter = 0;
   
   tag_map[Object] = get_tag();
   tag_map[IO] = get_tag();
   tag_map[Main] = get_tag();
   intclasstag = get_tag();
   tag_map[Int] = intclasstag;
   boolclasstag = get_tag();
   tag_map[Bool] = boolclasstag;
   stringclasstag = get_tag() ;
   tag_map[Str] = stringclasstag;
   
   
   enterscope();
   if (cgen_debug) cout << "Building CgenClassTable" << endl;
   install_basic_classes();
   install_classes(classes);
   build_inheritance_tree();

   code();
   exitscope();
}

void CgenClassTable::install_basic_classes()
{

// The tree package uses these globals to annotate the classes built below.
  //curr_lineno  = 0;
  Symbol filename = stringtable.add_string("<basic class>");

//
// A few special class names are installed in the lookup table but not
// the class list.  Thus, these classes exist, but are not part of the
// inheritance hierarchy.
// No_class serves as the parent of Object and the other special classes.
// SELF_TYPE is the self class; it cannot be redefined or inherited.
// prim_slot is a class known to the code generator.
//
  addid(No_class,
	new CgenNode(class_(No_class,No_class,nil_Features(),filename),
			    Basic,this));
  addid(SELF_TYPE,
	new CgenNode(class_(SELF_TYPE,No_class,nil_Features(),filename),
			    Basic,this));
  addid(prim_slot,
	new CgenNode(class_(prim_slot,No_class,nil_Features(),filename),
			    Basic,this));

// 
// The Object class has no parent class. Its methods are
//        cool_abort() : Object    aborts the program
//        type_name() : Str        returns a string representation of class name
//        copy() : SELF_TYPE       returns a copy of the object
//
// There is no need for method bodies in the basic classes---these
// are already built in to the runtime system.
//
  install_class(
   new CgenNode(
    class_(Object, 
	   No_class,
	   append_Features(
           append_Features(
           single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
           single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
           single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	   filename),
    Basic,this));

// 
// The IO class inherits from Object. Its methods are
//        out_string(Str) : SELF_TYPE          writes a string to the output
//        out_int(Int) : SELF_TYPE               "    an int    "  "     "
//        in_string() : Str                    reads a string from the input
//        in_int() : Int                         "   an int     "  "     "
//
   install_class(
    new CgenNode(
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
	   filename),	    
    Basic,this));

//
// The Int class has no methods and only a single attribute, the
// "val" for the integer. 
//
   install_class(
    new CgenNode(
     class_(Int, 
	    Object,
            single_Features(attr(val, prim_slot, no_expr())),
	    filename),
     Basic,this));

//
// Bool also has only the "val" slot.
//
    install_class(
     new CgenNode(
      class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename),
      Basic,this));

//
// The class Str has a number of slots and operations:
//       val                                  ???
//       str_field                            the string itself
//       length() : Int                       length of the string
//       concat(arg: Str) : Str               string concatenation
//       substr(arg: Int, arg2: Int): Str     substring
//       
   install_class(
    new CgenNode(
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
	     filename),
        Basic,this));

}

// CgenClassTable::install_class
// CgenClassTable::install_classes
//
// install_classes enters a list of classes in the symbol table.
//
void CgenClassTable::install_class(CgenNodeP nd)
{
  Symbol name = nd->get_name();

  if (probe(name))
    {
      return;
    }

  // The class name is legal, so add it to the list of classes
  // and the symbol table.
  nds = new List<CgenNode>(nd,nds);
  addid(name,nd);
}

void CgenClassTable::install_classes(Classes cs)
{
  for(int i = cs->first(); cs->more(i); i = cs->next(i))
    install_class(new CgenNode(cs->nth(i),NotBasic,this));
}

//
// CgenClassTable::build_inheritance_tree
//
void CgenClassTable::build_inheritance_tree()
{
  for(List<CgenNode> *l = nds; l; l = l->tl())
      set_relations(l->hd());
}

//
// CgenClassTable::set_relations
//
// Takes a CgenNode and locates its, and its parent's, inheritance nodes
// via the class table.  Parent and child pointers are added as appropriate.
//
void CgenClassTable::set_relations(CgenNodeP nd)
{
  CgenNode *parent_node = probe(nd->get_parent());
  nd->set_parentnd(parent_node);
  parent_node->add_child(nd);
}

void CgenNode::add_child(CgenNodeP n)
{
  children = new List<CgenNode>(n,children);
}

void CgenNode::set_parentnd(CgenNodeP p)
{
  assert(parentnd == NULL);
  assert(p != NULL);
  parentnd = p;
}

void CgenClassTable::code_name_tab_help(Symbol s) {
StringEntry* ptr;
  if ((ptr = stringtable.lookup_string(s->get_string())) != NULL) {
          str << WORD;
          ptr->code_ref(str);
          str << endl;
  }

}


void CgenClassTable::code_name_tab(CgenNodeP l) 
{
  Symbol s = l->name;
  StringEntry* ptr;
  if (!(s == Object || s == Main || s == IO || s == Int || s == Str || s == Bool)) {
  if ((ptr = stringtable.lookup_string(l->name->get_string())) != NULL) {
          str << WORD;
          ptr->code_ref(str);
          str << endl;
  }
  }
  for (List<CgenNode> *p = l->get_children(); p; p = p->tl()) 
  {
	code_name_tab(p->hd());
  }
}

void CgenClassTable::code_obj_tab_help(Symbol s) {
   str << WORD;
    str << s << PROTOBJ_SUFFIX;
    str << endl;
    str << WORD;
    str << s << CLASSINIT_SUFFIX;
    str << endl;
}

void CgenClassTable::code_obj_tab(CgenNodeP l, std::vector<Symbol>& que, int idx) 
{
  Symbol s = l->name;
  if (!(s == Object || s == Main || s == IO || s == Int || s == Str || s == Bool)) {
  str << WORD;
    str << s << PROTOBJ_SUFFIX;
    str << endl;
    str << WORD;
    str << s << CLASSINIT_SUFFIX;
    str << endl;
   }
  for (List<CgenNode> *p = l->get_children(); p; p = p->tl())
  {
        code_obj_tab(p->hd(), que, idx++);
  }
}

int CgenClassTable::code_disp_tab_helper(CgenNode* l, sym_int_type* smap, int cnt, std::map<Symbol, Symbol>* name_map) 
{
   if (l->get_parentnd() != NULL) {
	cnt = code_disp_tab_helper(l->get_parentnd(), smap, cnt, name_map);
   }
    for (int i = l->features->first(); l->features->more(i);
                i = l->features->next(i)) {
        if (!dynamic_cast<method_class*>(l->features->nth(i))) 
		continue;
        if (name_map->count(l->features->nth(i)->get_name()) != 1)
		continue;


        str << WORD;
        str << (*name_map)[l->features->nth(i)->get_name()] << ".";
        str << l->features->nth(i)->get_name()->get_string();

         (*name_map).erase(l->features->nth(i)->get_name());
        smap->insert(std::pair<Symbol, int>(l->features->nth(i)->get_name(), cnt));
        cnt++;
        str << endl;
    }
    return cnt;
}


void CgenClassTable::make_name_map(CgenNodeP l, std::map<Symbol, Symbol>*name_map) 
{
   for (int i = l->features->first(); l->features->more(i);
                i = l->features->next(i)) {
        if (!dynamic_cast<method_class*>(l->features->nth(i)))
                continue;

	if (name_map->count(l->features->nth(i)->get_name()) == 1) {
	   continue;
	}
//	std::cout << l->name << " method: " << l->features->nth(i)->get_name() << endl;
        (*name_map)[l->features->nth(i)->get_name()] = l->name;
    }
    if (l->get_parentnd() != NULL) {
        make_name_map(l->get_parentnd(), name_map);
   }
}

/*void CgenClassTable::make_method_queue(CgenNodeP l, std::map<Symbol, Symbol>& name_map, std::vect<std::pair<Symbol, Symbol> >& method_queue) {
  if (l->get_parentnd() != NULL) {
        make_method_queue(l->get_parentnd(), name_map, method_queue);
   }

  for (int i = l->features->first(); l->features->more(i);
                i = l->features->next(i)) {
        if (!dynamic_cast<method_class*>(l->features->nth(i)))
                continue;

        if (name_map->count(l->features->nth(i)->get_name()) == 0) {
           continue;
        }
   
        
   }
}*/

void CgenClassTable::code_disp_tab(CgenNodeP l) 
{
 // std::cout << "ddd: " << l->name << endl;
  std::map<Symbol, Symbol> name_map;
  make_name_map(l, &name_map);
  
  //std::vect<std::pair<Symbol, Symbol> > method_queue;
  //make_method_queue(l, name_map, method_queue);

  str << l->name->get_string() << DISPTAB_SUFFIX << LABEL;
  sym_int_type smap;
  code_disp_tab_helper(l, &smap, 0, &name_map);
  disp_map[l->name] = smap;
  for(List<CgenNode> *p = l->get_children(); p; p = p->tl())
  {
    code_disp_tab(p->hd());    
  }
}

int CgenClassTable::code_attr_helper(CgenNodeP l, sym_int_type* smap, int tag_idx)
{
   if (l->get_parentnd() != NULL) {
        tag_idx = code_attr_helper(l->get_parentnd(), smap, tag_idx);
   }
  for (int i = l->features->first(); l->features->more(i);
                i = l->features->next(i)) {
        attr_class* ptr;
        if (!(ptr = dynamic_cast<attr_class*>(l->features->nth(i))))
                continue;
        str << WORD;
        std::string type = ptr->type_decl->get_string();
        if (type == "Int") {
          inttable.lookup_string("0")->code_ref(str);
        } else if (type == "String") {
          stringtable.lookup_string("")->code_ref(str);
        } else if (type == "Bool") {
          falsebool.code_ref(str);
        } else {
          str << "0";
        }
        str << endl;
        (*smap)[ptr->name] = tag_idx;
        tag_idx++;
    }
  return tag_idx;
}

int count_attr_num(CgenNodeP l)
{
  int num = 0;
  for (int i = l->features->first(); l->features->more(i);
                i = l->features->next(i)) {
        if (!(dynamic_cast<attr_class*>(l->features->nth(i))))
                continue;
     num++;
  }

  if (l->get_parentnd() != NULL) {
        num += count_attr_num(l->get_parentnd());
   }
  return num;
}

void CgenClassTable::code_prot_obj(CgenNodeP l)
{
 /* int num = 0;
  for (int i = l->features->first(); l->features->more(i);
                i = l->features->next(i)) {
        if (!(dynamic_cast<attr_class*>(l->features->nth(i))))
                continue;
     num++;
  }*/
  int num = count_attr_num(l);
  str << WORD << "-1" << endl;
  str << l->name->get_string() << PROTOBJ_SUFFIX << LABEL;
//  str << WORD << tag_map[l->name] << endl;
  if (tag_map.count(l->name) == 1) {
	str << WORD << tag_map[l->name] << endl;
  } else {
	int d = get_tag();
    str << WORD << d << endl;
    tag_map[l->name] = d;
  }
/*  if (l->name == Int) {
    intclasstag = get_tag();
    str << WORD << intclasstag << endl;
    tag_map[l->name] = intclasstag;
  } else if (l->name == Str) {
    stringclasstag = get_tag();
    str << WORD << stringclasstag << endl;
    tag_map[l->name] = stringclasstag;
  } else if (l->name == Bool) {
    boolclasstag = get_tag();
    str << WORD << boolclasstag << endl;
    tag_map[l->name] = boolclasstag;
  } else {
    int d = get_tag();
    str << WORD << d << endl;
    tag_map[l->name] = d;
  }*/
  str << WORD << (DEFAULT_OBJFIELDS + num) << endl;
  str << WORD << l->name->get_string() << DISPTAB_SUFFIX << endl;

  sym_int_type smap;
 // int tag_idx = 0;
/*  for (int i = l->features->first(); l->features->more(i);
                i = l->features->next(i)) {
        attr_class* ptr;
        if (!(ptr = dynamic_cast<attr_class*>(l->features->nth(i))))
                continue;
        str << WORD;
	std::string type = ptr->type_decl->get_string();
        if (type == "Int") {
          inttable.lookup_string("0")->code_ref(str);
	} else if (type == "String") {
	  stringtable.lookup_string("")->code_ref(str);
	} else if (type == "Bool") {
	  falsebool.code_ref(str);
	} else {
	  str << "0";
	} 
        str << endl;
	smap[ptr->name] = tag_idx;
        tag_idx++;
    }*/
  code_attr_helper(l, &smap, 0);
  attr_map[l->name] = smap;
  for(List<CgenNode> *p = l->get_children(); p; p = p->tl())
  {
    code_prot_obj(p->hd());        
  }
}

void CgenClassTable::code_obj_init(CgenNodeP l)
{
  so = l->name;
  str << l->name->get_string() << CLASSINIT_SUFFIX << LABEL;
  emit_addiu(SP,SP,-12,str);
  emit_store(FP, 3, SP, str);
  emit_store(SELF, 2, SP, str);
  emit_store(RA, 1, SP, str);
  emit_addiu(FP, SP, 4, str);
  emit_move(SELF, ACC, str);
  if (l->get_parentnd() != NULL && l->get_parentnd()->name != No_class) {
     std::string s = (std::string)(l->get_parentnd()->name->get_string()) + (std::string)("_init");
     emit_jal(const_cast<char*>(s.c_str()), str);
  }

  for (int i = l->features->first(); l->features->more(i);
                i = l->features->next(i)) {
        attr_class* ptr;
        if (!(ptr = dynamic_cast<attr_class*>(l->features->nth(i))))
                continue;

           no_expr_class* neptr;
   	   if ((neptr = dynamic_cast<no_expr_class*>(ptr->init)) != NULL) {
      		if (ptr->type_decl == Int || ptr->type_decl == Str || ptr->type_decl == Bool) {	
			continue;
    	   } else {
	  		continue;
		}

	   }
   	ptr->init->code(str);
  	int att_idx = get_attr_idx(so, ptr->name);
   	emit_store(ACC, att_idx + 3, SELF, str);
   }

  emit_move(ACC, SELF, str);
  emit_load(FP, 3, SP, str);
  emit_load(SELF, 2, SP, str);
  emit_load(RA, 1, SP, str);
  emit_addiu(SP, SP, 12, str);
  emit_return(str);
  
  for(List<CgenNode> *p = l->get_children(); p; p = p->tl())
  {
    code_obj_init(p->hd());
  }
}

int get_method_idx(Symbol type, Symbol name) 
{
  if (disp_map.count(type) == 1) {
	if (disp_map[type].count(name) == 1) {
		return disp_map[type][name];
	}
  }
  return -1;
}

int get_attr_idx(Symbol type, Symbol name) 
{
  if (attr_map.count(type) == 1) {
        if (attr_map[type].count(name) == 1) {
                return attr_map[type][name];
        }
  }
  return -1;
}

void CgenClassTable::code_method(CgenNodeP l) 
{
  local_scope.enterscope();
  so = l->name;
  if (!(l->name == Object || l->name == Int || l->name == IO || l->name == Str || l->name == Bool)) {
    for (int i = l->features->first(); l->features->more(i);
                i = l->features->next(i)) {
         method_class* mptr;
         if (!(mptr = dynamic_cast<method_class*>(l->features->nth(i))))
                continue;
         local_scope.enterscope();
	int formal_num = 0;
         for (int j = mptr->formals->first(); mptr->formals->more(j);
		j = mptr->formals->next(j)) {
		formal_class* fptr;
		if (!(fptr = dynamic_cast<formal_class*>(mptr->formals->nth(j))))
                	continue;
		struct offset* t = new struct offset(0, j);
		local_scope.addid(fptr->name, t);
		formal_num++;
	}
         
         str << l->name->get_string() << "." << mptr->name->get_string() << LABEL;  

         emit_addiu(SP, SP, -12, str);
         emit_store(FP, 3, SP, str);
         emit_store(SELF, 2, SP, str);
         emit_store(RA, 1, SP, str);
         emit_addiu(FP, SP, 4, str);
         
         // real action
         emit_move(SELF, ACC, str);
         mptr->expr->code(str);

         emit_load(FP, 3, SP, str);
         emit_load(SELF, 2, SP, str);
         emit_load(RA, 1, SP, str);
	 emit_addiu(SP, SP, (DEFAULT_OBJFIELDS + formal_num) *  WORD_SIZE , str);
	 emit_return(str);

          local_scope.exitscope();
    }
  }
 
  local_scope.exitscope();
  for(List<CgenNode> *p = l->get_children(); p; p = p->tl())
  {
    code_method(p->hd());
  } 
}

void CgenClassTable::code_tag(CgenNodeP l)
{
  if (l->name == Int) {
    intclasstag = get_tag();
    tag_map[l->name] = intclasstag;
  } else if (l->name == Str) {
    stringclasstag = get_tag();
    tag_map[l->name] = stringclasstag;
  } else if (l->name == Bool) {
    boolclasstag = get_tag();
    tag_map[l->name] = boolclasstag;
  } else {
    int d = get_tag();
    tag_map[l->name] = d;
  }

  for(List<CgenNode> *p = l->get_children(); p; p = p->tl())
  {
    code_prot_obj(p->hd());
  }
}

void make_sym_node_map(CgenNodeP l) 
{
   //std::cout << l->name << endl;
    sym_node_map[l->name] = l;
    for(List<CgenNode> *p = l->get_children(); p; p = p->tl())
  {
    make_sym_node_map(p->hd());
  }
}

void CgenClassTable::code()
{
   make_sym_node_map(root());
  //code_tag(root());
  if (cgen_debug) cout << "coding global data" << endl;
  code_global_data();

  if (cgen_debug) cout << "choosing gc" << endl;
  code_select_gc();

  if (cgen_debug) cout << "coding constants" << endl;
  code_constants();

//                 Add your code to emit
//                   - prototype objects
//                   - class_nameTab
//                   - dispatch tables
//
  str << CLASSNAMETAB  << LABEL;
  code_name_tab_help(Object);
  code_name_tab_help(IO); 
  code_name_tab_help(Main);
  code_name_tab_help(Int);
  code_name_tab_help(Bool);
  code_name_tab_help(Str);
  code_name_tab(root());

  std::vector<Symbol> que(tag_map.size());
   for (std::map<Symbol, int>::const_iterator it = tag_map.begin(); it != tag_map.end(); it++) {
        que[it->second] = it->first;
   }
  str << CLASSOBJTAB << LABEL;
  code_obj_tab_help(Object);
  code_obj_tab_help(IO);
  code_obj_tab_help(Main);
  code_obj_tab_help(Int);
  code_obj_tab_help(Bool);
  code_obj_tab_help(Str);
  code_obj_tab(root(), que, 0);

  code_disp_tab(root());
  code_prot_obj(root());

  if (cgen_debug) cout << "coding global text" << endl;
  code_global_text();

//                 Add your code to emit
//                   - object initializer
//                   - the class methods
//                   - etc...
  code_obj_init(root());
  code_method(root());
}


CgenNodeP CgenClassTable::root()
{
   return probe(Object);
}


///////////////////////////////////////////////////////////////////////
//
// CgenNode methods
//
///////////////////////////////////////////////////////////////////////

CgenNode::CgenNode(Class_ nd, Basicness bstatus, CgenClassTableP ct) :
   class__class((const class__class &) *nd),
   parentnd(NULL),
   children(NULL),
   basic_status(bstatus),
   tag(tag)
{ 
   stringtable.add_string(name->get_string());          // Add class name to string table
}


//******************************************************************
//
//   Fill in the following methods to produce code for the
//   appropriate expression.  You may add or remove parameters
//   as you wish, but if you do, remember to change the parameters
//   of the declarations in `cool-tree.h'  Sample code for
//   constant integers, strings, and booleans are provided.
//
//*****************************************************************

void assign_class::code(ostream &s) {
   expr->code(s);

/*   if (name == self) {
        emit_move(ACC, SELF, s);
        return;
   }*/

   struct offset * ptr;
   if ((ptr = local_scope.lookup(name)) != NULL) {
        // it's local var
        if (ptr->type == 1) {
                emit_store(ACC, -(ptr->pos), FP, s);
        } else {
                emit_store(ACC, ptr->pos + 3, FP, s);
        }
        return;
   }


   int idx = get_attr_idx(so, name);
   emit_store(ACC, idx + 3, SELF, s);  

//   emit_addiu(A1, SELF, 12, s);
//   emit_jal("_GenGC_Assign" ,s);

}

void static_dispatch_class::code(ostream &s) {
   //int arg_num = actual->len() - 1;
    //s << "num is " << arg_num << endl;
   if (type_name == Str && name == substr) {
        int arg_num = actual->len() - 1;
        int ii = 0;
        while (ii <= arg_num) {
        actual->nth(ii)->code(s);
        emit_store(ACC, 0, SP, s);
        emit_addiu(SP, SP, -4, s);
        ii++;
        }
    } else {
       int arg_num = actual->len();
        emit_addiu(SP, SP, -4 * arg_num, s);
         int ii = 0;
        while ( ii < arg_num) {
        actual->nth(ii)->code(s);
        emit_store(ACC, (ii + 1) , SP, s);
        ii++;
        }
    }

 
    expr->code(s);
    std::string label_str("label");
    int nt1 = label_num;
    label_str += nt1;
    label_num++;
    emit_bne(ACC, ZERO, nt1, s);
    //!!! need to check the object   
    emit_load_string(ACC, stringtable.lookup(0), s);
    emit_load_imm(T1, 1, s);
    emit_jal(DISP_ABORT, s);

    emit_label_ref(nt1, s);
    s << LABEL;



    s << LA << T1 << " ";
    emit_disptable_ref(type_name, s); s<< endl;   
    int method_idx = -1;
   
    if (type_name == SELF_TYPE) {
        method_idx = get_method_idx(so, name);
    } else {
        method_idx = get_method_idx(type_name, name);
    }

    emit_load(T1, method_idx, T1, s);
    emit_jalr(T1, s);

}

void dispatch_class::code(ostream &s) {
    if (name == substr) {
        int arg_num = actual->len() - 1;
        int ii = 0;
        while (ii <= arg_num) {
        actual->nth(ii)->code(s);
        emit_store(ACC, 0, SP, s);
        emit_addiu(SP, SP, -4, s);
        ii++;
        }
    } else {
	int arg_num = actual->len();
        emit_addiu(SP, SP, -4 * arg_num, s);
         int ii = 0;
        while ( ii < arg_num) {
        actual->nth(ii)->code(s);
        emit_store(ACC, (ii + 1) , SP, s);
        ii++;
        }
    }
    /*int arg_num = actual->len() - 1;
    //s << "num is " << arg_num << endl;
    if (expr->get_type() == Str && name == substr) {
	int ii = 0;
	while (ii <= arg_num) {
        actual->nth(ii)->code(s);
        emit_store(ACC, 0, SP, s);
        emit_addiu(SP, SP, -4, s);
        ii++;
        }
    } else {
        int ii = 0;
	while (ii <= arg_num) {
        actual->nth(ii)->code(s);
        emit_store(ACC, 0, SP, s);
        emit_addiu(SP, SP, -4, s);
        ii++;
        }
    }*/
    expr->code(s);
    std::string label_str("label");
    int nt1 = label_num;
    label_str += nt1;
    label_num++;
    emit_bne(ACC, ZERO, nt1, s);
    //!!! need to check the object   
    emit_load_string(ACC, stringtable.lookup(0), s);
    emit_load_imm(T1, 1, s);
    emit_jal(DISP_ABORT, s);

    emit_label_ref(nt1, s);
    s << LABEL;

    Symbol type = expr->get_type();
    int method_idx = -1;
    if (type == SELF_TYPE) {
	method_idx = get_method_idx(so, name);
    } else {
	method_idx = get_method_idx(type, name);
    }
    emit_load(T1, 2, ACC, s);
    
    emit_load(T1, method_idx, T1, s);
    emit_jalr(T1, s);
}

void cond_class::code(ostream &s) {
    int else_num = label_num++;
    int end_num = label_num++;
    pred->code(s);
    emit_load(T1, 3, ACC, s);
    emit_beqz(T1, else_num, s);
    then_exp->code(s);
    emit_branch(end_num, s);
    
    s << "label" << else_num << LABEL;
    else_exp->code(s);
    
    s << "label" << end_num << LABEL;    
}

void loop_class::code(ostream &s) {
    int loop_label = label_num++;
    int end_label = label_num++;

    s << "label" << loop_label << LABEL;
    pred->code(s);
    emit_load(T1, 3, ACC, s);
    emit_beq(T1, ZERO, end_label, s);
    body->code(s);
    emit_branch(loop_label, s);
   
    s << "label" << end_label << LABEL;
    emit_move(ACC, ZERO, s);
}

int find_low_bound(Symbol s) 
{
  int d = tag_map[s];
  CgenNodeP l = sym_node_map[s];
  
  for(List<CgenNode> *p = l->get_children(); p; p = p->tl())
  {
    d = std::max(d, find_low_bound(p->hd()->name));
  }
  return d;
}

void typcase_class::code(ostream &s) {
   int end_num = label_num++;
   int start_num = label_num++;
   expr->code(s);

   //emit_store(ACC, 0, SP, s);
   //emit_addiu(SP, SP, -4, s);
    emit_bne(ACC, ZERO, start_num, s);
    //!!! need to check the object   
    emit_load_string(ACC, stringtable.lookup(0), s);
    emit_load_imm(T1, 1, s);
    emit_jal(CASE_ABORT2, s);
   
    emit_label_ref(start_num, s);  s << LABEL;
    emit_load(T2, 0, ACC, s);
    
    std::map<int, int> tag_idx_map;
    std::map<int, Symbol> idx_sym_map;
    int max_tag = -1;
    int min_tag = 2147483647;
    for (int i = cases->first(); cases->more(i);
                i = cases->next(i)) {
        branch_class* bc;
        if (!(bc = dynamic_cast<branch_class*>(cases->nth(i))))
                continue;

 	int obj_tag_idx = tag_map[bc->type_decl];
        tag_idx_map[obj_tag_idx] = i;
        idx_sym_map[i] = bc->type_decl;
        max_tag = std::max(max_tag, obj_tag_idx);
        min_tag = std::min(min_tag, obj_tag_idx);
    } 
    for (int i = max_tag; i >= min_tag; i--) {
        if (tag_idx_map.count(i) == 0)
		continue;
	int j = tag_idx_map[i];
        branch_class* bc;
        if (!(bc = dynamic_cast<branch_class*>(cases->nth(j))))
                continue;

        local_scope.enterscope();
        //emit_store(ACC, 0, SP, s);
        //emit_addiu(SP, SP, -4, s);
        //local_scope.addid(bc->name, new offset(1, local_num++));
   
        int t_num = label_num;
        emit_blti(T2, i, label_num, s);
        emit_bgti(T2, find_low_bound(idx_sym_map[j]), label_num, s);
        label_num++;  

        emit_store(ACC, 0, SP, s);
        emit_addiu(SP, SP, -4, s);
        local_scope.addid(bc->name, new offset(1, local_num++));        
  
        bc->expr->code(s);
        emit_addiu(SP, SP, 4, s);
        local_num--;
        local_scope.exitscope();
        emit_branch(end_num, s);
        emit_label_ref(t_num, s);  s << LABEL;
    }
    emit_jal(CASE_ABORT, s);

    emit_label_ref(end_num, s);  s << LABEL;
}

void block_class::code(ostream &s) {
   for (int i = body->first(); body->more(i); i = body->next(i)) {
	body->nth(i)->code(s);
   }
}

void let_class::code(ostream &s) {
    local_scope.enterscope();
   
    no_expr_class* neptr;
    if ((neptr = dynamic_cast<no_expr_class*>(init)) != NULL) {
        if (type_decl == Bool) {
	    emit_load_bool(ACC, falsebool, s);
        } else if (type_decl == Int) {
	    emit_load_int(ACC, inttable.lookup_string("0"), s);
	} else if (type_decl == Str) {
	    emit_load_string(ACC, stringtable.lookup_string(""), s);
        } else {
	   init->code(s);
	}
     } else {
	init->code(s);
     }  
   // init->code(s);
    
    emit_store(ACC, 0, SP, s);
    local_scope.addid(identifier, new offset(1, local_num++));
    emit_addiu(SP, SP, -4, s);
    body->code(s);
    emit_addiu(SP, SP, 4, s);
    
    local_num--;
    local_scope.exitscope();
}

void plus_class::code(ostream &s) {
    e1->code(s);
    emit_jal("Object.copy"  ,s);
    //now a0, is a copy of expr1
    
    emit_store(ACC, 0, SP, s);
    emit_addiu(SP, SP, -4, s);

    e2->code(s);
    emit_load(T1, 1, SP, s);    
    
    emit_load(T2, 3, T1, s);
    emit_load(T3, 3, ACC, s);
    emit_add(T2, T2, T3, s);

    emit_load(ACC, 1, SP, s);
    emit_store(T2, 3, ACC, s);
    emit_addiu(SP, SP, 4, s);
}

void sub_class::code(ostream &s) {
    e1->code(s);
    emit_jal("Object.copy"  ,s);
    //now a0, is a copy of expr1

    emit_store(ACC, 0, SP, s);
    emit_addiu(SP, SP, -4, s);

    e2->code(s);
    emit_load(T1, 1, SP, s);

    emit_load(T2, 3, T1, s);
    emit_load(T3, 3, ACC, s);
    emit_sub(T2, T2, T3, s);
   
    emit_load(ACC, 1, SP, s);
    emit_store(T2, 3, ACC, s);
    emit_addiu(SP, SP, 4, s);
}

void mul_class::code(ostream &s) {
    e1->code(s);
    emit_jal("Object.copy"  ,s);
    //now a0, is a copy of expr1

    emit_store(ACC, 0, SP, s);
    emit_addiu(SP, SP, -4, s);

    e2->code(s);
    emit_load(T1, 1, SP, s);

    emit_load(T2, 3, T1, s);
    emit_load(T3, 3, ACC, s);
    emit_mul(T2, T2, T3, s);

    emit_load(ACC, 1, SP, s);
    emit_store(T2, 3, ACC, s);
    emit_addiu(SP, SP, 4, s);
}

void divide_class::code(ostream &s) {
    e1->code(s);
    emit_jal("Object.copy"  ,s);
    //now a0, is a copy of expr1

    emit_store(ACC, 0, SP, s);
    emit_addiu(SP, SP, -4, s);

    e2->code(s);
    emit_load(T1, 1, SP, s);

    emit_load(T2, 3, T1, s);
    emit_load(T3, 3, ACC, s);

    emit_div(T2, T2, T3, s);

    emit_load(ACC, 1, SP, s);
    emit_store(T2, 3, ACC, s);
    emit_addiu(SP, SP, 4, s);
}

void neg_class::code(ostream &s) {
     e1->code(s);
     emit_jal("Object.copy"  ,s);
     emit_load(T1, 3, ACC, s);
     emit_neg(T1, T1, s);
     emit_store(T1,3, ACC, s); 
}

void lt_class::code(ostream &s) {
    e1->code(s);
     emit_store(ACC, 0, SP, s);
     emit_addiu(SP, SP, -4, s);
     e2->code(s);
     emit_load(T1, 1, SP, s);
     emit_move(T2, ACC, s);
     emit_load(T1, 3, T1, s);
     emit_load(T2, 3, T2, s);
     emit_addiu(SP, SP, 4, s);
     emit_load_bool(ACC, truebool, s);
     emit_blt(T1, T2, label_num, s);
     emit_load_bool(ACC, falsebool, s);
     s << "label" << label_num++ << LABEL;
}

void eq_class::code(ostream &s) {
     e1->code(s);
     emit_store(ACC, 0, SP, s);
     emit_addiu(SP, SP, -4, s);
     e2->code(s);
     emit_load(T1, 1, SP, s);
    emit_move(T2, ACC, s);
     emit_addiu(SP, SP, 4, s);
     emit_load_bool(ACC, truebool, s);
     emit_beq(T1, T2, label_num, s);
     emit_load_bool(A1, falsebool, s);
     emit_jal(eq_test , s);
     s << "label" << label_num++ << LABEL;
}

void leq_class::code(ostream &s) {
    e1->code(s);
    emit_store(ACC, 0, SP, s);
    emit_addiu(SP, SP, -4, s);
    e2->code(s);
    emit_load(T1, 1, SP, s);
    emit_move(T2, ACC, s);
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 3, T1, s);
    emit_load(T2, 3, T2, s);
    emit_load_bool(ACC, truebool, s);
     emit_bleq(T1, T2, label_num, s);
     emit_load_bool(ACC, falsebool, s);
    s << "label" << label_num++ << LABEL;
}

void comp_class::code(ostream &s) {
     e1->code(s);
     emit_load(T1, 3, ACC, s);
     emit_load_bool(ACC, truebool, s);
     emit_beqz(T1, label_num, s);
     emit_load_bool(ACC, falsebool, s);
     s << "label" << label_num++ << LABEL;
}

void int_const_class::code(ostream& s)  
{
  //
  // Need to be sure we have an IntEntry *, not an arbitrary Symbol
  //
  emit_load_int(ACC,inttable.lookup_string(token->get_string()),s);
}

void string_const_class::code(ostream& s)
{
  emit_load_string(ACC,stringtable.lookup_string(token->get_string()),s);
}

void bool_const_class::code(ostream& s)
{
  emit_load_bool(ACC, BoolConst(val), s);
}

void new__class::code(ostream &s) {
 //  std::string name = type_name->get_string();
   Symbol type;
   if (type_name == SELF_TYPE) {
	emit_load_address(T1, "class_objTab", s);
	emit_load(T2, 0, SELF,s);
	emit_sll(T2, T2, 3, s);
	emit_addu(T1, T1, T2, s);
	emit_store(T1, 0, SP, s);
        emit_addiu(SP, SP, -4, s);
	//emit_move(S1, T1, s);
	emit_load(ACC, 0, T1, s);
	
   } else {
	type = type_name;
	s << LA << ACC << " ";
   emit_protobj_ref(type, s);
   s << endl;
   }
   //emit_load_string(ACC, const_cast<char *>((name + PROTOBJ_SUFFIX).c_str()), s);
   emit_jal("Object.copy" , s);
   if (type_name == SELF_TYPE) { 
	emit_load(T1, 1, SP, s);
	emit_addiu(SP, SP, 4, s);
        emit_load(T1, 1, T1, s);
	emit_jalr(T1, s);
   } else {
	s << JAL;
   emit_init_ref(type, s);
   s << endl;
   }
}

void isvoid_class::code(ostream &s) {
    e1->code(s);
    emit_move(T1, ACC, s);
    emit_load_bool(ACC, truebool, s);
     emit_beqz(T1, label_num, s);
     emit_load_bool(ACC, falsebool, s);
     s << "label" << label_num++ << LABEL;
}

void no_expr_class::code(ostream &s) {
    emit_move(ACC, ZERO, s);
}

void object_class::code(ostream &s) {
   if (name == self) {
	emit_move(ACC, SELF, s);
	return;
   }

   struct offset * ptr;
   if ((ptr = local_scope.lookup(name)) != NULL) {
	// it's local var
	if (ptr->type == 1) {
		emit_load(ACC, -(ptr->pos), FP, s);
	} else {
		emit_load(ACC, ptr->pos + 3, FP, s);
	}
	return;
   } 


   int idx = get_attr_idx(so, name);
   emit_load(ACC, idx + 3, SELF, s);
}

