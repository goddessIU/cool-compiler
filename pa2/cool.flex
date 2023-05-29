/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */
/*
 *  for comment
 */
int nestnum;
int error_str();
void error_str_handle();
%}
%x str
%x linecomment
%x nestcomment
%x strlong
%x strnull
/*
 * Define names for regular expressions here.
 */
DARROW          =>
WHITESPACE      [ \t\r\v\f]+
DIGIT           [0-9]
LETTER          [a-zA-Z]
UPPER           [A-Z]
LOWER           [a-z]
ASSIGN          <-
LE              <=
%%


<nestcomment>--           { }

 /*
  * single line comment
  */
<INITIAL>--             { BEGIN(linecomment); }
<linecomment>.+  {}
<linecomment>\n   { curr_lineno++;BEGIN(INITIAL); }

 /*
  *  Nested comments
  */
"(*"             { nestnum = 1; BEGIN(nestcomment); }
<nestcomment>"(""*"   { nestnum++;  }
<nestcomment>[^*\n]    {}
<nestcomment>"*"+[^*)\n]*   {}
<nestcomment>\n  { curr_lineno++; }
<nestcomment>"*"+")"  { nestnum--; 
			if (nestnum == 0) {
				BEGIN(INITIAL);
			}}
<nestcomment><<EOF>> { char* t = "EOF in comment"; 
			cool_yylval.error_msg = t;
			BEGIN(INITIAL);
			return ERROR;
			}

 /*
  * the single-character  operator
  */
[,~@(){};:+-/<=]                     { return *yytext; }
"*"                               { return *yytext; }
 /*
  *  The multiple-character operators.
  */
{DARROW}		{ return (DARROW); }
{LE}                    { return LE; }
{ASSIGN}                { return ASSIGN; }
 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */
(?i:class)              { return (CLASS); }
(?i:else)               { return ELSE;    }
(?i:if)                 { return IF; }
(?i:fi)                 { return FI; }
(?i:in)                 { return IN; }
(?i:inherits)           { return INHERITS; }
(?i:let)                { return LET; }
(?i:loop)               { return LOOP; }
(?i:pool)               { return POOL; }
(?i:then)               { return THEN; }
(?i:while)              { return WHILE; }
(?i:case)               { return CASE; }
(?i:esac)               { return ESAC; }
(?i:of)                 { return OF; }
(?i:new)                { return NEW; }
(?i:isvoid)             { return ISVOID; }
(?i:not)                { return NOT; }
t(?i:rue)               { cool_yylval.boolean = 1; return BOOL_CONST; }
f(?i:alse)              { cool_yylval.boolean = 0; return BOOL_CONST; }
 
 
 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */
\"                    { string_buf_ptr = string_buf; BEGIN(str); }
<str>\"               {
		        BEGIN(INITIAL);
			if (error_str() == 1) {
				error_str_handle();
				return ERROR;
			}
			*string_buf_ptr = '\0';
			cool_yylval.symbol = stringtable.add_string(string_buf, MAX_STR_CONST);
			return STR_CONST;
		      }
<str>\n              {
			cool_yylval.error_msg = "Unterminated string constant";
			curr_lineno++;
			BEGIN(INITIAL);
			return ERROR;
			}
<str>\0              {
			cool_yylval.error_msg = "String contains null character";
			BEGIN(INITIAL);
			BEGIN(strnull);
			return ERROR;
			}
<str>\\n                {
				if (error_str() == 1) { error_str_handle(); return ERROR; }
				*string_buf_ptr++ = '\n';}

			   
<str>\\t		{
				if (error_str() == 1) { error_str_handle(); return ERROR; }
				*string_buf_ptr++ = '\t';}
<str>\\b		{       
				if (error_str() == 1) { error_str_handle(); return ERROR; }
				*string_buf_ptr++ = '\b';}
<str>\\f		{
				if (error_str() == 1) { error_str_handle(); return ERROR; }
				*string_buf_ptr++ = '\f';}

<str>\\(\n)                  { curr_lineno++; 	
				if (error_str() == 1) { error_str_handle(); return ERROR; }
				*string_buf_ptr++ = '\n';}
<str>\\(.)		{
				if (error_str() == 1) { error_str_handle(); return ERROR; }
				*string_buf_ptr++ = yytext[1];}
<str>[^\\\n\"]+      {
			char *yptr = yytext;
			while (*yptr){ 
				if (error_str() == 1) {	
					error_str_handle();
					return ERROR;
				};
				*string_buf_ptr++ = *yptr++;
				}
			}
<str><<EOF>>          { char* t = "EOF in string constant"; 
			cool_yylval.error_msg = t;
			BEGIN(INITIAL);
			return ERROR;
			}


<strlong>\"         {  BEGIN(INITIAL); }
<strlong>\n         { curr_lineno++; BEGIN(INITIAL);  }
<strlong>[^\"\n]*    {}

<strnull>\"          { BEGIN(INITIAL);}
<strnull>\n          { curr_lineno++; BEGIN(INITIAL); }
<strnull>[^\"\n]*    {}
 /*
  * id and int
  */
[A-Z][a-zA-Z0-9_]*   { cool_yylval.symbol = idtable.add_string(yytext); return TYPEID;}
[a-z][a-zA-Z0-9_]*   { cool_yylval.symbol = idtable.add_string(yytext); return OBJECTID; }
{DIGIT}+                { cool_yylval.symbol = inttable.add_string(yytext); return INT_CONST; }

 /*
  * whitespace
  */ 
 \n                  { curr_lineno++; }
 {WHITESPACE}       {}

 /*
  * error
  */
"*)"                 { cool_yylval.error_msg = "Unmatched *)"; return ERROR; }
.                    { cool_yylval.error_msg = yytext; return ERROR; }
<INITIAL><<EOR>>              { yyterminate(); }
%%

int error_str() {
	if (string_buf_ptr - string_buf > MAX_STR_CONST - 1){
		return 1;
	} else {
		return 0;
	}
}

void error_str_handle() {
	BEGIN(INITIAL);
	BEGIN(strlong);
	cool_yylval.error_msg = "String constant too long";
	
}
