%{
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "y.tab.h"

int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}

void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}

/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/

/* @function: getstr
 * @input: a string literal
 * @output: the string value for the input which has all the escape sequences
 * translated into their meaning.
 */
char *getstr(const char *str)
{
    int length = strlen(str);
    if (str[0] != '\"' || str[length - 1] != '\"') {
        /* check if str is enclosed by " */
        fprintf(stderr, "string not enclosed by \"\n");
        return NULL;
    }

    if (length == 2) {
        return "";
    }

    char *ptr = (char *)str + 1;
    char *ptr_end = (char *)str + length - 1;
    char *value = (char *)checked_malloc(length - 1);
    char *value_ptr = value;

    while (ptr != ptr_end) {
        char ch = *ptr;

        if (*ptr == '\\') {
            char *end;
            switch (*(ptr + 1)) {
                case '\\':
                    ch = '\\';
                    ptr += 2;
                    break;
                case '\"':
                    ch = '\"';
                    ptr += 2;
                    break;
                case 'x':
                    ch = strtol(ptr + 2, &end, 16);
                    ptr = end;
                    break;
                case '0': case '1': case '2': case '3': case '4': case '5':
                case '6': case '7': case '8': case '9':
                    ch = strtol(ptr + 1, &end, 8);
                    ptr = end;
                    break;
                case 'a':
                    ch = '\a';
                    ptr += 2;
                    break;
                case 'b':
                    ch = '\b';
                    ptr += 2;
                    break;
                case 'f':
                    ch = '\f';
                    ptr += 2;
                    break;
                case 'n':
                    ch = '\n';
                    ptr += 2;
                    break;
                case 'r':
                    ch = '\r';
                    ptr += 2;
                    break;
                case 't':
                    ch = '\t';
                    ptr += 2;
                    break;
                case 'v':
                    ch = '\v';
                    ptr += 2;
                    break;
                default:
                    fprintf(stderr, "invalid escape value \\%c\n", *(ptr + 1));
                    free(value);
                    return NULL;
            }

        } else {
            ptr ++;
        }

        *value_ptr = ch;
        value_ptr ++;
    }

    *value_ptr = '\0';
    return value;
}
%}

%S COMMENT

  /* lex definitions */
alphabets     [a-zA-Z]

control_char      \\[abfnrtv]
oct_char      \\([0-3]?[0-7]{1,2})
hex_char      \\x[0-9a-fA-F]{1,2}
escape       {control_char}|{oct_char}|{hex_char}|\\\\|\\\"

    int nested_comments = 0;
%%
  /*
  * begining of the rule
  * Below is an example, which you can wipe out
  * and write reguler expressions and actions of your own.
  */

 /* reserved words */
<INITIAL>if       { adjust(); return IF;}
<INITIAL>then     { adjust(); return THEN;}
<INITIAL>else     { adjust(); return ELSE;}
<INITIAL>do       { adjust(); return DO;}
<INITIAL>to       { adjust(); return TO;}
<INITIAL>while    { adjust(); return WHILE;}
<INITIAL>for      { adjust(); return FOR;}
<INITIAL>break    { adjust(); return BREAK;}
<INITIAL>let      { adjust(); return LET;}
<INITIAL>in       { adjust(); return IN;}
<INITIAL>end      { adjust(); return END;}
<INITIAL>function { adjust(); return FUNCTION;}
<INITIAL>var      { adjust(); return VAR;}
<INITIAL>type     { adjust(); return TYPE;}
<INITIAL>array    { adjust(); return ARRAY;}
<INITIAL>of       { adjust(); return OF;}
<INITIAL>nil      { adjust(); return NIL;}


  /* identifier */
<INITIAL>[a-zA-Z][a-zA-Z0-9_]* {
    adjust();
    yylval.sval = String(yytext);
    return ID;
}

<INITIAL>[0-9]+              {
    adjust();
    yylval.ival = atoi(yytext);
    return INT;
}

  /* string */
<INITIAL>\"({escape}|[^"\134])*\" {
    adjust();
    yylval.sval = getstr(yytext);
    return STRING;
}
  /* string error */
<INITIAL>\"[^"]*\"    {
    adjust();
    EM_error(EM_tokPos, "illegal string: %s", yytext);
}

 /* punctuation symbol */
<INITIAL>","      { adjust(); return COMMA;}
<INITIAL>":"      { adjust(); return COLON;}
<INITIAL>";"      { adjust(); return SEMICOLON;}
<INITIAL>"("      { adjust(); return LPAREN;}
<INITIAL>")"      { adjust(); return RPAREN;}
<INITIAL>"["      { adjust(); return LBRACK;}
<INITIAL>"]"      { adjust(); return RBRACK;}
<INITIAL>"{"      { adjust(); return LBRACE; }
<INITIAL>"}"      { adjust(); return RBRACE; }
<INITIAL>"."      { adjust(); return DOT; }
<INITIAL>"+"      { adjust(); return PLUS; }
<INITIAL>"-"      { adjust(); return MINUS; }
<INITIAL>"*"      { adjust(); return TIMES; }
<INITIAL>"/"      { adjust(); return DIVIDE; }
<INITIAL>"="      { adjust(); return EQ;}
<INITIAL>"<>"     { adjust(); return NEQ;}
<INITIAL>"<"      { adjust(); return LT;}
<INITIAL>"<="     { adjust(); return LE;}
<INITIAL>">"      { adjust(); return GT;}
<INITIAL>">="     { adjust(); return GE;}
<INITIAL>"&"      { adjust(); return AND;}
<INITIAL>"|"      { adjust(); return OR;}
<INITIAL>":="     { adjust(); return ASSIGN;}

  /* comment */
<INITIAL>"/*" {
    adjust();
    nested_comments = 1;
    BEGIN COMMENT;
}
<COMMENT>"/*" {
    adjust();
    nested_comments ++;
}
<COMMENT>"*/" {
    adjust();
    nested_comments --;
    if (nested_comments == 0) {
        BEGIN INITIAL;
    }
}
<COMMENT>.|\n {adjust();}
<COMMENT><<EOF>> {
    EM_error(EM_tokPos, "invalid comment, one or more '*/' missing");
    return 0;
}

<INITIAL>[ \t\n]+   { adjust(); }

<INITIAL>.        {adjust(); EM_error(EM_tokPos, "illegal character: %s", yytext);}
.|\n                 {BEGIN INITIAL; REJECT;}
