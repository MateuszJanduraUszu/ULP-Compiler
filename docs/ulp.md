# UFUI Language Pack (ULP)

The **.ulp** files serve as repositories for human-readable translations of messages, which are subsequently compiled into **.umc** files.

## Syntax

The syntax of ULP files is designed to be intuitive and straightforward. It typically follows this structure:

1. **Pack information**: This section includes essential details about the pack, such as the language and LCID (Locale ID). The language is specified using the [@language](#language) directive, while the LCID is set with the [@lcid](#lcid) directive.
2. **Metadata (Optional)**: Following the pack information, [metadata](#metadata) can be included to provide additional information about the pack. This metadata is enclosed within a [@meta](#meta) section and can include details like author information, version numbers, changelogs, and many more.
3. **Content**: The core of the ULP file is the content section, where all messages and groups are defined. This section is encapsulated within a [@content](#content) block and contains the actual translations of messages. Each message is assigned a unique ID preceded by a hash sign (`#`).
4. **Keywords**: Keywords in ULP files are prefixed with an at sign (`@`). These keywords define various aspects of the pack, such as language, LCID, and metadata.
5. **String literals**: Values, such as message translations, are stored as [string literals](#string-literals), which are enclosed within double quotation marks (`"`). These string literals support Unicode encoding, allowing for the representation of a wide range of characters.
6. **Comments**: [Comments](#comments) can be included throughout the file using double slashes (`//`). These comments are ignored during compilation and serve as annotations or notes for developers.
7. **Syntax elements**: All syntax elements, including keywords, message IDs, and string literals, must adhere the specific rules. For example, message IDs must follow naming conventions, and string literals must begin and end on the same line.

The overall structure of a ULP file is hierarchical, with pack information and metadata serving as overarching directives, and the content section containing nested groups and messages.

## Keywords

The ULP format includes several registered keywords:

### `@language`

Defines the language to which messages are translated. It must be the first directive used in the pack and follows this format:

```
@language: "<language>"
```

It is treated as Unicode, therefore, the language name can be specified in the desired language.

You can find more information about languages [here](#localization).

### `@lcid`

Defines the LCID (Locale ID) associated with the language. It is recommended to keep LCIDs in sync with the Windows LCIDs to enable
the auto-detection of the user's preferred message catalog from the installed ones. It must be the second directive in the pack,
immediately following the [@language](#language) directive; otherwise, an error is reported during compilation. The directive follows
this format:

```
@lcid: "<lcid>"
```

The LCID must be a decimal number ranging from **0** to **2147483647**.

You can find more information about LCIDs [here](#localization).

### `@meta`

Defines metadata for the pack. It is completely discarded during compilation, meaning that any errors within the meta section will not be
reported by the compiler. While this section is optional, if it is specified, it must be the third directive in the pack; otherwise, an error will
be reported. This directive follows this format:

```
@meta
{
    ...
}
```

> **Note** <br>
> Both `@meta` and `@content` must be enclosed within curly brackets. For example:
> ```
> @language: "Polski"
> @lcid: "1045"
> {
>     @meta
>     {
>         ...
>     }
>
>     @content
>     {
>         ...
>     }
> }
> ```

You can find more information about metadata [here](#metadata).

### `@content`

Defines the content of the pack, storing all messages and groups exported from the pack. Unlike [@meta](#meta), this section is
required, even if it is empty. It must be the fourth directive in the pack and follows this format:

```
@content
{
    ...
}
```

This section is also referred to as the **root group**.

You can find more information about root groups [here](#groups).

### `@group`

Defines a group of messages and subgroups. Each group must have a unique name within its scope. The name of the group must adhere
the following requirements: 

* Each character must be one of: lowercase letter (**a-z**), uppercase letter (**A-Z**), digit (**0-9**), '**-**' or '**_**'.
* The name cannot contain spaces or whitespaces between words.
* The name must be unique within its scope.

The group directive follows this format:

```
@group: "<name>"
{
    ...
}
```

You can find more information about groups [here](#groups).

## Localization

Localization comprises two key components: language name and LCID (Locale ID). The language name represents the target language to
which the messages are translated. Meanwhile, the LCID denotes the specific language locale. The LCID plays a crucial role in
automatically suggesting the message catalog from installed options, ensuring compatibility with the user's preferred language.
However, it's important to note that the LCID provided is merely a hint and does not necessarily have to correspond to an existing LCID.

## Metadata

Metadata stores information about the pack. It can contain a wide range of details, including but not limited to, author/authors, pack
version, changelog, and other relevant information. Similar to comments, metadata is completely discarded during compilation, meaning
that any errors or inconsistencies within it will not be reported. Despite this, metadata is valuable for development purposes as it
provides essential context and documentation for the pack.

## Groups

Groups serve as containers for organizing messages and subgroups, providing contextual organization within the ULP file. Without
groups, messages would need to be organized individually, leading to potential clutter and difficulty in managing related items.

For example, without groups, you might organize messages like this:

```
#widget-button-yes: "Tak"
#widget-button-no: "Nie"
#widget-button-cancel: "Anuluj"
#widget-button-apply: "Zastosuj"
```

However, with groups, you can achieve a more organized structure:

```
@group: "widget"
{
    @group: "button"
    {
        #yes: "Tak"
        #no: "Nie"
        #cancel: "Anuluj"
        #apply: "Zastosuj"
    }
}
```

Each group has a unique name within its scope, ensuring clarity and avoiding naming conflicts. For instance:

```
@group: "group-name-1"
{
    @group: "subgroup-name-1" {}
    @group: "subgroup-name-2" {}
}

@group: "group-name-2"
{
    // these names are valid because their parent is "group-name-2", not "group-name-1"
    @group: "subgroup-name-1" {}
    @group: "subgroup-name-2" {}
}

@group: "group-name-3"
{
    @group: "subgroup-name-1" {}
    @group: "subgroup-name-1" {} // Error: 'subgroup-name-1' already defined
}
```

Groups can be empty, but this may result in a warning or error depending on the error model being used.

## Messages

Messages store translated text that will be used as language-specific messages. They consist of two elements: ID and value.

* **ID**: Identifies the message from other messages. Each ID is unique within its scope, ensuring clarity and avoiding naming conflicts.
* **Value**: Stores translated text in Unicode encoding.

Messages can be placed both in the root group (`@content`) and normal groups (`@group`). The only difference is that messages from
the root group don't have a group name prefix in the message name.

To access a message, you must specify its full name, including parent groups. For example:

```
#widget-button-cancel: "Anuluj"

@group: "widget"
{
    @group: "button"
    {
        #yes: "Tak"
        #no: "Nie"
    }
}
```

To access `#yes`, you must specify the name `widget.button#yes`. Child groups are connected with the parent group via a dot (`.`), and
messages are always connected with a group via a hash sign (`#`). Because `#widget-button-cancel` is in the root group, to access it you
just need to specify the name `#widget-button-cancel`; the group name is redundant here.

Messages also support format arguments, which follow this format: `{%n}`, where `n` indicates the argument index. The index starts with
0, and its maximum value is 999. Here are a few examples:

```
{%0}      // Valid, expects an argument with index 0
{%19}     // Valid, expects an argument with index 19
{%952}    // Valid, expects an argument with index 952
{%001}    // Valid, expects an argument with index 1
{% 5}     // Invalid, brackets, percent, and digits must be contiguous
{ % 2}    // Invalid, brackets, percent, and digits must be contiguous
{%25 }    // Invalid, brackets, percent, and digits must be contiguous
{%ff}     // Invalid, only digits are allowed
```

## String literals

String literals contain the value of items you want to define. By default, they are treated as Unicode, allowing you to store almost
anything as long as Unicode can handle it. String literals are enclosed within quotes.

> Note <br>
> When including a quote as a part of a string literal, you must place a backslash (`\`) before it. Otherwise, the compiler will interpret it as
two separate string literals. Additionally, string literals must begin and end on the same line. Here's an example:
> ```
> "Hello World!"        // This is fine
> "Hello "World"!"      // Error: Compiler interprets it as 'Hello ', 'World', and '!'
> "Hello "World!"       // Error: Compiler interprets it as 'Hello ' and 'World!'. Also, a missing closing quote will be reported.
> "Hello \"World\"!"    // This is fine. Compiler interprets it as a single string literal: 'Hello "World"!'
> ```

Multi-line string literals are also supported. Each string literal represents a single line. For example:

```
#msg-id: "This is the first line."
         "This is the second line."
         "This is the third line."
```

The code will compile into a single sentence, with each line concatenated using an end-of-line character. For example, after compilation,
the following will be displayed:

```
This is the first line.
This is the second line.
This is the third line.
```

## Comments

Comments are blocks of text that begin with a double-slash (`//`) and extend to the end of the line. These comments are completely
ignored during compilation but serve as annotations or notes within the file. They are valuable for providing context, explanations, or
documentation for the code, which makes it easier for developers to understand.

```
@language: "Polski" // This is my comment, it is ignored
// this is also my comment, which is also ignored
@lcid: "1045"
// @lcid: "1033" <- Ignored, because it is a comment
...
```