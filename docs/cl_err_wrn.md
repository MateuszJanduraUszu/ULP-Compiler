# Compiler errors and warnings

## In this document
* [Compiler errors](#compiler-errors)
    * [File errors](#file-errors)
    * [Lexical analysis/parsing errors](#lexical-analysisparsing-errors)
    * [Compilation errors](#compilation-errors)
    * [Symbol file errors](#symbol-file-errors)
* [Compiler warnings](#compiler-warnings)
    * [File warnings](#file-warnings)
    * [Lexical analysis/parsing warnings](#lexical-analysisparsing-warnings)
    * [Compilation warnings](#compilation-warnings)
    * [Symbol file warnings](#symbol-file-warnings)

## Compiler errors

### File errors

* `E1000`: cannot open the input file 's'

    Occurs when the compiler is unable to open the specified file.

* `E1001`: input file 's' is empty

    Occurs when the error model is set to strict, and the specified input file is empty.

* `E1002`: detected unsupported encoding

    Occurs when the compiler detects an unsupported encoding in the input file, typically due to the presence of a [BOM](https://en.wikipedia.org/wiki/Byte_order_mark) that is not UTF-8 BOM.

### Lexical analysis/parsing errors

* `E2000`: undefined symbol 's' which is required

    Occurs when a required symbol, such as `@language`, `@lcid`, or `@content`, is not undefined within the input file.

    ```
    @language: "Polski"
    { // '@lcid' is undefined because it should be defined after '@language', E2000 reported
        @content
        {
        }
    }
    ```

* `E2001`: missing opening bracket '{' for the global section

    Occurs when the opening bracket '{' is not specified for the global section.

    ```
    @language: "Polski"
    @lcid: "1033"
        @content // missing opening bracket '{' which should be before '@content', E2001 reported
        {
        }
    }
    ```

* `E2002`: missing closing bracket '}' for the global section

    Occurs when the closing bracket '}' is not specified for the global section.

    ```
    @language: "Polski"
    @lcid: "1033"
    {
        @content
        {
        }
    // missing closing bracket '}' which should be the last token, E2002 reported
    ```

* `E2003`: missing opening bracket '{' for group 's'

    Occurs when the opening bracket '{' is not specified for the group.

    ```
    @group: "group-name"
        #msg-id: "msg-value" // missing opending bracket '{' which should be before '#msg-id', E2003 reported 
    }
    ```

* `E2004`: missing closing bracket '}' for group 's'

    Occurs when the closing bracket '}' is not specified for the group.

    ```
    @group: "group-name"
    {
        #msg-id: "msg-value"
    // missing closing bracket '}', E2004 reported
    ```

* `E2005`: incomplete message 's'

    Occurs when the message is incomplete, i.e., some token is missing or tokens are in the wrong order.

    ```
    #msg-id // missing colon ':' and string literal '""' which are required for messages, E2005 reported
    ```

* `E2006`: invalid usage of the 's' keyword

    Occurs when the keyword was used incorrectly, i.e., some token is missing or tokens are in the wrong order.

    ```
    @group // missing colon ':' and opening bracket '{' which are required for 'group' keyword, E2006 reported
    {
    }
    ```

* `E2007`: ambiguous group name, 's' is already defined

    Occurs when the group is already defined in the current scope.

    ```
    @group: "parent-group-name"
    {
        @group: "group-name"
        {}
    
        @group: "group-name" // 'group-name' is already defined, E2007 reported
        {}
    }
    ```

* `E2008`: ambiguous identifier name, 's' is already defined

    Occurs when the identifier is already defined in the current scope.

    ```
    #msg-id: "msg-value"
    #msg-id: "other-msg-value" // '#msg-id' is already defined, E2008 reported
    ```

* `E2009`: illegal group name 's'

    Occurs when the group name is illegal, i.e., contains forbidden characters or has the wrong format.

    ```
    #Идентификатор-сообщения: "msg-value" // 'Идентификатор-сообщения' is not a legal name, E2009 reported
    ```

* `E2010`: illegal identifier name 's'

    Occurs when the identifier name is illegal, i.e., contains forbidden characters or has the wrong format.

    ```
    #訊息ID: "msg-value" // '訊息ID' is not a legal name, E2010 reported
    ```

* `E2011`: invalid '@lcid' value

    Occurs when the value specified in `@lcid` is invalid, i.e., contains non-digits.

    ```
    @lcid: "MY_LCID" // 'MY_LCID' is not a decimal number, E2011 reported
    ```

* `E2012`: unexpected token 's'

    Occurs when the token is unexpected in the current context.

    ```
    #msg-id: "msg-value" {} // brackets are unexpected here, E2012 reported
    ```

* `E2013`: pack 's' has no messages or groups

    Occurs when the pack has no messages or groups and the error model is set to strict.

    ```
    @language: "Polski"
    @lcid: "1033"
    {
        @content
        {
            // '@content' has no messages or groups, E2013 reported
        }
    }
    ```

* `E2014`: message 's' has an empty value

    Occurs when the message has an empty value and the error model is set to strict.

    ```
    #msg-id: "" // '#msg-id' has empty value, E2014 reported
    ```

* `E2015`: group 's' has no members

    Occurs when the group has no members and the error model is set to strict.

    ```
    @group: "group-name"
    {
        // 'group-name' has no members, E2015 reported
    }
    ```

* `E2016`: missing closing quote '"' for string literal

    Occurs when the string literal has no closing quote.

    ```
    #msg-id: "msg-value // missing closing quote '}', E2016 reported
    ```

### Compilation errors

* `E3000`: cannot create the UMC file 's'

    Occurs when the compiler is unable to create the specified UMC file.

* `E3001`: cannot open the UMC file 's'

    Occurs when the compiler is unable to open the specified UMC file for overwrite.

* `E3002`: cannot generate the UMC file header

    Occurs when the compiler is unable to generate a header for the specified UMC file.

* `E3003`: cannot generate the UMC file lookup table

    Occurs when the compiler is unable to generate a lookup table for the specified UMC file.

* `E3004`: cannot generate the UMC file blob

    Occurs when the compiler is unable to generate a blob for the specified UMC file.

### Symbol file errors

* `E4000`: cannot create the symbol file 's'

    Occurs when the compiler is unable to create the specified symbol file.

* `E4001`: cannot open the symbol file 's'

    Occurs when the compiler is unable to create the specified symbol file for overwrite.

* `E4002`: cannot write symbol to the symbol file 's'

    Occurs when the compiler is unable to write a symbol to the specified symbol file.

## Compiler warnings

### File warnings

### Lexical analysis/parsing warnings

### Compilation warnings

### Symbol file warnings