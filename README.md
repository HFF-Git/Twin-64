# SPL64
A system programming language compiler for the Twin-64 System. The idea is to have a higher level way to write low level code instead of pure assembler language. The compiler will produce an assembler file for the assembler to process.


## Scalar types

```
<typ>               ->  <scalar-typ> | <structured-typ>

<scalar-typ>        ->  "BYTE" | "SHORT" | "WORD" | "INT"

<array-typ>         ->  "ARRAY" <const-expr> "OF" <typ>
                        "END

<struct-typ>        ->  "STRUCT" [ "(" <ident> ")" ]
                        [ <field-list> ]
                        "END"    

<field-list>        ->  { <ident> ":" <typ> ";" }
```

## Declarations

```
<const-decl>        ->  "CONST" <ident> "=" <expr> ";"

<typ-decl>          ->  "TYPE"  <ident> "=" <typ"> ";"

<var-decl>          ->  "VAR"   <ident> [ "=" <value> ] ";"

<func-decl>         ->  "FUNC" <ident> 
                        "<parm-list>
                        [ ":" <typ> ] ";"
                        <stmt-seq>
                        "END"

<parm-list>         -> "(" [ <ident> ":" <type> ] { "," <ident> ":" <type> } ")"

<module-decl>       ->  "MODULE"
                        [ <import-decl> ] 
                        [ <export-decl> ]
                        [ <var-decl> ]
                        [ <const-decl> ]
                        [ <typ-decl> ]
                        [ <func-dec> ]
                        "BEGIN"
                        <stmt-seq>
                        "END"

<import-decl>       ->  "IMPORT <ident> { "," <indet> } ";"

<export-decl>       ->  "EXPORT <ident> { "," <indet> } ";"

```

## Expressions

```
<expr>              ->  <simple-expr> <rel-op> <simple-expr>

<simple-expr>       ->  [ "+" | "-" ] <term> { <termOp> <term> }

<termOp>            ->  "+" | “-“ | “OR“ | "XOR"

<term>              ->  <factor> { <facOp> <factor> }

<facOp>             ->  "*" | “/“ | “%“ | "AND"

<factor>            ->  <val> |
                        <ident> |
                        <reg> |
                        "~" <factor> |
                        "(" <expr> ")" |
                        <func-call>

<val>               ->  "<numeric> | 
                        "L%" <val> | 
                        "R%" <val>

<adr-ref>           ->  "@" <ident>

<func-call>         ->  <ident> [ arg-list] ";"

<arg-list>         ->  "(" [ <expr> ] { "," <expr> } ")"

```

## Statements

```
<stmt-seq>          ->  <stmt> { ";" <stmt> } 

<stmt>              ->  <assign-stmt<> |
                        <if-stmt> | 
                        <loop-stmt> |
                        <with-stmt> |

<assign-stmt>       ->  <ident> ":=" <expr>

<if-stmt>           ->  "IF" <expr> "THEN" <stmt-seq>
                        { "ELSEIF" <expr> "THEN" <stmt-seq> }
                        [ "ELSE" <stmt-seq> ]
                        "END"

<loop-stmt>         ->  "LOOP" 
                        <loop-stmt-seq> 
                        "END

<loop-stmt-seq>     ->  <stmt-seq> | "CONTINUE" ";" | "BREAK" ";"

<with-stmt>         ->  "WITH" <adr> "DO" <stmt-seq> "END"

```

## Example

```
MODULE example 

    IMPORT aModule;
    EXPORTS ;

    CONST maxItem = 10;

    TYPE listEntry = STRUCT

        field1 : WORD;
        field2 : BYTE;
    END

    TYPE payLoadEntry = STRUCT ( listEntry )

        field3 : INT;
    END

    FUNC add2 ( in1 : WORD; in2: WORD ) : WORD;

        add2 = in1 + in2;
    END
END

```

