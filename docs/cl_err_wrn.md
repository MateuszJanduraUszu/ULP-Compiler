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

    Occurs when the [error model](compiler.md#error-model) is set to strict, and the specified input file is empty.

* `E1002`: detected unsupported encoding

    Occurs when the compiler detects an unsupported encoding in the input file, typically due to the presence of a [BOM](https://en.wikipedia.org/wiki/Byte_order_mark) that is not UTF-8 BOM.

### Lexical analysis/parsing errors

* `E2000`: undefined symbol 's' which is required

    ```
    @language: "Polski"
    { // '@lcid' is undefined because it should be defined after '@language', E2000 reported
        @content
        {
        }
    }
    ```

* `E2001`: missing opening bracket '{' for the global section

    ```
    @language: "Polski"
    @lcid: "1033"
        @content // missing opening bracket '{' which should be before '@content', E2001 reported
        {
        }
    }
    ```

* `E2002`: missing closing bracket '}' for the global section

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

    ```
    @group: "group-name"
        #msg-id: "msg-value" // missing opending bracket '{' which should be before '#msg-id', E2003 reported 
    }
    ```

* `E2004`: missing closing bracket '}' for group 's'

    ```
    @group: "group-name"
    {
        #msg-id: "msg-value"
    // missing closing bracket '}', E2004 reported
    ```

* `E2005`: incomplete message 's'

    ```
    #msg-id // missing colon ':' and string literal '""' which are required for messages, E2005 reported
    ```

* `E2006`: invalid usage of the 's' keyword

    ```
    @group // missing colon ':' and opening bracket '{' which are required for 'group' keyword, E2006 reported
    {
    }
    ```

* `E2007`: ambiguous group name, 's' is already defined

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

    ```
    #msg-id: "msg-value"
    #msg-id: "other-msg-value" // '#msg-id' is already defined, E2008 reported
    ```

* `E2009`: illegal group name 's'

    ```
    #Идентификатор-сообщения: "msg-value" // 'Идентификатор-сообщения' is not a legal name, E2009 reported
    ```

* `E2010`: illegal identifier name 's'

    ```
    #訊息ID: "msg-value" // '訊息ID' is not a legal name, E2010 reported
    ```

* `E2011`: invalid '@lcid' value

    ```
    @lcid: "MY_LCID" // 'MY_LCID' is not a decimal number, E2011 reported
    ```

* `E2012`: unexpected token 's'

    ```
    #msg-id: "msg-value" {} // brackets are unexpected here, E2012 reported
    ```

* `E2013`: pack 's' has no messages or groups

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

    ```
    #msg-id: "" // '#msg-id' has empty value, E2014 reported
    ```

* `E2015`: group 's' has no members

    ```
    @group: "group-name"
    {
        // 'group-name' has no members, E2015 reported
    }
    ```

* `E2016`: missing closing quote '"' for string literal

    ```
    #msg-id: "msg-value // missing closing quote '}', E2016 reported
    ```

### Compilation errors

### Symbol file errors

## Compiler warnings

### File warnings

### Lexical analysis/parsing warnings

### Compilation warnings

### Symbol file warnings