+ ##### handle comments: 
    ```
    Use a STATE called 'COMMENT' to handle comment, 
    whenever there is '/*' then enter 'COMMENT' STATE.
    There is a global variable 'nested_comments' to indicate level of nested comment.
    ```
+ ##### handle string
    ```
    Use one regular expression to describe string:
        \"(\\[abfnrtv]|\\[0-3]?[0-7]{1,2}|\\x[0-9a-fA-F]{1,2}|"\\"|\\\"|[^"\134])*\"
    A more generic regular expression:
        \"[^"]*\" 
    is used to handle string with errors
    ```

+ ##### error handling
    ```
    Any character does not match any rule will cause a error.
    A 'COMMENT' STATE util EOF will cause a error about nested comment.
    ```

+ ##### end of file handling
    ```
    Function yywrap() will be called when Lex reach end of file, and yywrap() return 1
    to tell Lex continues normal wrapup and stop.
    Additionally, there is a rule when 'COMMENT' STATE reach EOF, it echos the error message 
    and return 0, which indicate end-of-file.
    ```

+ ##### other
    ```
    nothing so interesting
    ```

