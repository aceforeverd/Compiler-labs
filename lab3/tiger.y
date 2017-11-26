/* declarations */
%{
#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"

int yylex(void); /* function prototype */

A_exp absyn_root;

void yyerror(char *s)
{
 EM_error(EM_tokPos, "%s", s);
}
%}


%union {
	int pos;
	int ival;
	string sval;
	A_exp exp;
	A_expList explist;
	A_var var;
	A_decList declist;
	A_dec dec;
	A_efieldList efieldlist;
	A_efield  efield;
	A_namety namety;
	A_nametyList nametylist;
    A_field a_field;
	A_fieldList fieldlist;
	A_field field;
	A_fundecList fundeclist;
	A_fundec fundec;
	A_ty ty;
}

%token <sval> ID STRING
%token <ival> INT

%token
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK
  LBRACE RBRACE DOT
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE

%type <exp> exp expseq op_exp arrayexp
%type <explist> params params_nonempty exps exps_nonempty
%type <var>  lvalue
%type <declist> decs decs_nonempty
%type <dec> vardec dec
%type <efieldlist> fields_assign fields_ass_nonempty
%type <efield> field_assign
%type <nametylist> tydec
%type <namety>  tydec_one
%type <a_field> a_field
%type <fieldlist> typefields tyfields_nonempty
%type <ty> ty
%type <fundeclist> fundec
%type <fundec> fundec_one
%type <sval> typeid

%nonassoc ASSIGN WHILE IF FOR ELSE
%left DOT
%left OR
%left AND
%nonassoc GE LE GT LT EQ NEQ
%left PLUS MINUS
%left TIMES DIVIDE
%left LBRACK RBRACK

%start program

%%
/* rules */

program : exp
          {
              absyn_root = $1;
          }
        ;

exp     :   NIL
            {
                $$ = A_NilExp(EM_tokPos);
            }
        |   INT
            {
                $$ = A_IntExp(EM_tokPos, $1);
            }
        |   STRING
            {
                $$ = A_StringExp(EM_tokPos, $1);
            }
        |   arrayexp
            {
                $$ = $1;
            }
        |   ID LBRACE fields_assign RBRACE
            {
                $$ = A_RecordExp(EM_tokPos, S_Symbol($1), $3);
            }
        |   lvalue
            {
                $$ = A_VarExp(EM_tokPos, $1);
            }
        |   ID params
            {
                $$ = A_CallExp(EM_tokPos, S_Symbol($1), $2);
            }
        /* |   lvalue DOT ID LPAREN params RPAREN */
        |   op_exp
            {
                $$ = $1;
            }
        |   LPAREN RPAREN
            {
                $$ = A_SeqExp(EM_tokPos, NULL);
            }
        |   LPAREN exps RPAREN
            {
                A_expList list = $2;
                if (list->tail == NULL) {
                    $$ = list->head;
                } else {
                    $$ = A_SeqExp(EM_tokPos, $2);
                }
            }
            /* assignment */
        |   lvalue ASSIGN exp
            {
                $$ = A_AssignExp(EM_tokPos, $1, $3);
            }
        |   IF exp THEN exp
            {
                $$ = A_IfExp(EM_tokPos, $2, $4, NULL);
            }
        |   exp AND exp
            {
                $$ = A_IfExp(EM_tokPos, $1, $3, A_IntExp(EM_tokPos, 0));
            }
        |   IF exp THEN exp ELSE exp
            {
                $$ = A_IfExp(EM_tokPos, $2, $4, $6);
            }
        |   exp OR exp
            {
                $$ = A_IfExp(EM_tokPos, $1, A_IntExp(EM_tokPos, 1), $3);
            }
        |   WHILE exp DO exp
            {
                $$ = A_WhileExp(EM_tokPos, $2, $4);
            }
        |   FOR ID ASSIGN exp TO exp DO exp
            {
                $$ = A_ForExp(EM_tokPos, S_Symbol($2), $4, $6, $8);
            }
        |   BREAK
            {
                $$ = A_BreakExp(EM_tokPos);
            }
        |   LET decs IN expseq END
            {
                $$ = A_LetExp(EM_tokPos, $2, $4);
            }
        ;

arrayexp :  ID LBRACK exp RBRACK OF exp
            {
                $$ = A_ArrayExp(EM_tokPos, S_Symbol($1), $3, $6);
            }
         ;

op_exp  : exp PLUS exp
            {
                $$ = A_OpExp(EM_tokPos, A_plusOp, $1, $3);
            }
        | exp MINUS exp
            {
                $$ = A_OpExp(EM_tokPos, A_minusOp, $1, $3);
            }
        | exp TIMES exp
            {
                $$ = A_OpExp(EM_tokPos, A_timesOp, $1, $3);
            }
        | exp DIVIDE exp
            {
                $$ = A_OpExp(EM_tokPos, A_divideOp, $1, $3);
            }
        | exp GT exp
            {
                $$ = A_OpExp(EM_tokPos, A_gtOp, $1, $3);
            }
        | exp LT exp
            {
                $$ = A_OpExp(EM_tokPos, A_ltOp, $1, $3);
            }
        | exp GE exp
            {
                $$ = A_OpExp(EM_tokPos, A_geOp, $1, $3);
            }
        | exp LE exp
            {
                $$ = A_OpExp(EM_tokPos, A_leOp, $1, $3);
            }
        | exp EQ exp
            {
                $$ = A_OpExp(EM_tokPos, A_eqOp, $1, $3);
            }
        | exp NEQ exp
            {
                $$ = A_OpExp(EM_tokPos, A_neqOp, $1, $3);
            }
        | MINUS exp
            {
                $$ = A_OpExp(EM_tokPos, A_minusOp, A_IntExp(EM_tokPos, 0), $2);
            }
        ;

expseq : exps
         {
             $$ = A_SeqExp(EM_tokPos, $1);
         }
       ;

params   : LPAREN RPAREN
           {
               $$ = NULL;
           }
         | LPAREN params_nonempty RPAREN
            {
                $$ = $2;
            }
         ;

params_nonempty  : exp
                    {
                        $$ = A_ExpList($1, NULL);
                    }
                 | exp COMMA params_nonempty
                    {
                        $$ = A_ExpList($1, $3);
                    }
                 ;

fields_assign : /* empty */
                {
                  $$ = NULL;
                }
              | fields_ass_nonempty
                {
                    $$ = $1;
                }
              ;

fields_ass_nonempty : field_assign
                        {
                            $$ = A_EfieldList($1, NULL);
                        }
                    | field_assign COMMA fields_ass_nonempty
                        {
                            $$ = A_EfieldList($1, $3);
                        }
                    ;

field_assign : ID EQ exp
                {
                    $$ = A_Efield(S_Symbol($1), $3);
                }
             ;

ty         : typeid
               {
                   $$ = A_NameTy(EM_tokPos, S_Symbol($1));
               }
           | LBRACE typefields RBRACE
               {
                   $$ = A_RecordTy(EM_tokPos, $2);
               }
           | ARRAY OF typeid
               {
                   $$ = A_ArrayTy(EM_tokPos, S_Symbol($3));
               }
           ;

typefields : /* empty */
             {
                 $$ = NULL;
             }
           | tyfields_nonempty
             {
                 $$ = $1;
             }
           ;

a_field : ID COLON typeid
              {
                  $$ = A_Field(EM_tokPos, S_Symbol($1), S_Symbol($3));
              }
        ;

tyfields_nonempty : a_field
                        {
                            $$ = A_FieldList($1, NULL);
                        }
                  | a_field COMMA tyfields_nonempty
                        {
                            $$ = A_FieldList($1, $3);
                        }
                  ;

typeid : ID
           {
                $$ = $1;
           }
       ;

decs : /* empty */
         {
             $$ = NULL;
         }
     | decs_nonempty
         {
             $$ = $1;
         }
     ;

decs_nonempty : dec
                  {
                      $$ = A_DecList($1, NULL);
                  }
              | dec decs_nonempty
                  {
                      $$ = A_DecList($1, $2);
                  }
              ;


dec        : tydec
               {
                   $$ = A_TypeDec(EM_tokPos, $1);
               }
           | vardec
               {
                   $$ = $1;
               }
           | fundec
               {
                   $$ = A_FunctionDec(EM_tokPos, $1);
               }
           ;

tydec :    tydec_one
            {
                $$ = A_NametyList($1, NULL);
            }
      |    tydec_one tydec
            {
                $$ = A_NametyList($1, $2);
            }
      ;

tydec_one:  TYPE ID EQ ty
            {
                $$ = A_Namety(S_Symbol($2), $4);
            }
         ;

fundec  : fundec_one
            {
                $$ = A_FundecList($1, NULL);
            }
        | fundec_one fundec
            {
                $$ = A_FundecList($1, $2);
            }
        ;

fundec_one : FUNCTION ID LPAREN typefields RPAREN EQ exp
             {
                 $$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, S_Symbol(""), $7);
             }
           | FUNCTION ID LPAREN typefields RPAREN COLON typeid EQ exp
             {
                $$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, S_Symbol($7), $9);
             }
           ;

vardec : VAR ID ASSIGN exp
            {
                /* if ($4 == 'nil') { */
                /*     EM_error(EM_tokPos, "var type required for nil"); */
                /* } */
                $$ = A_VarDec(EM_tokPos, S_Symbol($2), S_Symbol(""), $4);
            }
       | VAR ID COLON typeid ASSIGN exp
            {
                $$ = A_VarDec(EM_tokPos, S_Symbol($2), S_Symbol($4), $6);
            }
       ;

lvalue     : ID
               {
                   $$ = A_SimpleVar(EM_tokPos, S_Symbol($1));
               }
           | ID LBRACK exp RBRACK
               {
                   $$ = A_SubscriptVar(EM_tokPos, A_SimpleVar(EM_tokPos, S_Symbol($1)), $3);
               }
           | lvalue DOT ID
               {
                   $$ = A_FieldVar(EM_tokPos, $1, S_Symbol($3));
               }
           | lvalue LBRACK exp RBRACK
               {
                   $$ = A_SubscriptVar(EM_tokPos, $1, $3);
               }
           ;

exps       : /* empty */
             {
                $$ = NULL;
             }
           | exps_nonempty
             {
                 $$ = $1;
             }
           ;

exps_nonempty : exp
                {
                    $$ = A_ExpList($1, NULL);
                }
              | exp SEMICOLON exps_nonempty
                {
                    $$ = A_ExpList($1, $3);
                }
              ;

