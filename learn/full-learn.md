<!-- 01-intro/readme.md -->
# Chapter 1: What is CCO?

After this chapter you will know:
- What CCO is
- How it differs from JSON, INI, YAML
- What a complete `.cco` file looks like

## 1.1 In One Sentence

**CCO** = **C**NT **C**onfiguration **O**bject.

It is a **human-readable configuration file format**, in the same category as JSON, YAML, and INI. But CCO does a few things they cannot:

- Define **type aliases** (e.g. `$typedef.Url: String`)
- Define **enumerations** (e.g. `$enum.Mode = (DEV, STAGING, PROD)`)
- Define **templates** (like "classes" or "structs" in programming languages)
- Write **expressions** inside configs (e.g. `$(10 + 20 * 3)`)

## 1.2 A Quick Preview

```cco
/* A web server configuration */

$typedef.Url: String,
$enum.Mode = (DEV, STAGING, PROD),

$temp.Server: (
    host: String,
    port: Integer = 8080,
    mode: Mode = DEV
),

app: (
    name: "libcco-demo",
    version: 0.1,
    servers: (
        #Server("localhost"),
        #Server("api.example.com", 443, PROD)
    )
)
```

Do not worry if none of this makes sense yet -- after the tutorial it will feel simpler than JSON.

## 1.3 CCO vs Other Formats

| Need | JSON | YAML | CCO |
|------|------|------|-----|
| Comments | no | `#` supported | `/* */` supported |
| Type aliases | no | no | `$typedef` |
| Enumerations | no | no | `$enum` |
| Reusable structures | no | no | `$temp` |
| Expressions | no | no | `$(...)` |
| Learning curve | low | medium | low-medium |
| Expressiveness | weak | moderate | strong |

## 1.4 What You Will Be Able to Do

1. Read any `.cco` configuration file
2. Write well-structured `.cco` files yourself
3. Use the libcco C library to parse and generate `.cco` files (that is a separate document)

Next chapter -- your first `.cco` file.


<!-- 02-first-file/readme.md -->
# Chapter 2: Your First .cco File

After this chapter you will be able to:
- Write the simplest `.cco` file
- Understand the basic structure of a `.cco` file

## 2.1 The Shortest Config File

Open an editor, create `hello.cco`, and type:

```cco
name: "world"
```

Save it. This is a valid `.cco` file.

## 2.2 What Does It Mean?

- `name` -- a key (a label)
- `:` -- a colon separating the key and value
- `"world"` -- a value (a string in this case)

The line means: **"set key name to value world"**.

## 2.3 Multiple Keys

```cco
name: "libcco",
version: 1
```

There are two key-value pairs, separated by a **comma `,`** .

## 2.4 Important Rules

- A key and value must be separated by a colon `:`
- Multiple key-value pairs must be separated by commas `,`
- Strings must be wrapped in double quotes `"..."`

## 2.5 Common Mistakes

```cco
/* NOTE: missing colon -- ERROR */
name "world"

/* NOTE: missing comma -- ERROR */
name: "libcco"
version: 1

/* NOTE: unquoted string -- ERROR */
name: world
```

```cco
/* NOTE: correct */
name: "libcco",
version: 1
```

## 2.6 Summary

- The basic unit in CCO is a `key: value` pair
- Multiple pairs are separated by commas
- Strings must be wrapped in `"..."`

Next chapter -- basic data types.


<!-- 03-basic-data/readme.md -->
# Chapter 3: Basic Data Types

After this chapter you will be able to write all of CCO's primitive values.

## 3.1 None (Null)

Equivalent to `null` or `nil` in other languages:

```cco
nothing: None
```

The keyword `None` (capitalized) means "no value".

## 3.2 Boolean (True/False)

Only two values:

```cco
debug: true,
production: false
```

The keywords `true` and `false` are lowercase.

## 3.3 Integers

Plain integers:

```cco
count: 42,
negative: -10
```

CCO also supports other bases and digit separators:

| Writing | Meaning | Actual Value |
|---------|---------|--------------|
| `42` | decimal | 42 |
| `0xFF` | hexadecimal (`0x` prefix) | 255 |
| `0o10` | octal (`0o` prefix) | 8 |
| `0b1010` | binary (`0b` prefix) | 10 |
| `1_000_000` | underscore separator | 1000000 |

Underscores `_` are for readability only:

```cco
large_number: 1_000_000_000
```

## 3.4 Floats

Numbers with a decimal point:

```cco
pi: 3.14,
avogadro: 6.02e23,
small: 1.0
```

Scientific notation via `e` (or `E`) is supported:

```cco
price: 1.99e10
```

## 3.5 Strings

Wrapped in double quotes:

```cco
greeting: "Hello, world!"
```

Strings support escape sequences:

| Sequence | Meaning |
|----------|---------|
| `\n` | newline |
| `\t` | tab |
| `\\` | backslash |
| `\"` | double quote |
| `\r` | carriage return |
| `\xNN` | hex byte (e.g. `\x41` = 'A') |
| `\uNNNN` | Unicode character (e.g. `\u0041` = 'A') |

```cco
message: "line one\nline two",
path: "C:\\Program Files\\app"
```

### Raw Strings (Backticks)

If you want to avoid escape processing, use backticks `` ` ``. Everything inside is literal:

```cco
regex: `\d+\.\d+`,
multiline: `first line
second line
third line`
```

Backtick strings **do not process any escapes** -- `\n` is two characters (backslash + n).

## 3.6 Summary

| Type | Example | Notes |
|------|---------|-------|
| None | `None` | null value |
| Boolean | `true`, `false` | true/false |
| Integer | `42`, `0xFF`, `0b1010`, `1_000` | multiple bases, underscores |
| Float | `3.14`, `1e10` | scientific notation |
| String | `"hello"`, `` `raw` `` | double-quote with escapes, backtick for raw |

Next chapter -- comments.


<!-- 04-comments/readme.md -->
# Chapter 4: Comments

After this chapter you will be able to annotate `.cco` files with explanations.

## 4.1 CCO Comments

CCO supports one comment style: **C-style block comments** `/* ... */`.

```cco
/* NOTE: this is a comment */
name: "world"
```

Comments can span multiple lines:

```cco
/*
 * A longer explanation.
 * Describes the purpose of the config below.
 */
server: "localhost",
port: 8080
```

Comments can appear between values:

```cco
name: "libcco",   /* NOTE: application name */
version: 0.1      /* NOTE: version number */
```

## 4.2 Comparison with JSON

```json
{
    /* NOTE: JSON does NOT support comments! */
    "name": "libcco"
}
```

JSON does not support comments. YAML supports `#` line comments. INI supports `;` and `#`. CCO uses `/* */`, just like C.

## 4.3 Comparison with YAML

```yaml
# YAML uses this kind of comment
name: libcco
```

CCO does not use `#` for comments because `#` has another purpose in CCO (template instantiation, covered later).

## 4.4 Tips

- Comments can appear wherever whitespace is allowed
- Comments **cannot nest** (`/* /* */ */` will error)
- Use comments to explain **why**, not **what** (the code already says what)

## 4.5 Summary

- Start with `/*`, end with `*/`
- Can span multiple lines
- Line comments (`//`) are **not** supported

Next chapter -- maps (key-value pairs).


<!-- 05-maps/readme.md -->
# Chapter 5: Maps -- Key-Value Pairs

After this chapter you will be able to group related data using CCO's maps.

## 5.1 Remember Chapter 2's Syntax?

```cco
name: "libcco",
version: 1
```

This is actually a shorthand (detailed in Chapter 7). The full form is:

```cco
(name: "libcco", version: 1)
```

Wrapped in parentheses `()` or braces `{}`, with `key: value` pairs separated by commas -- this is CCO's **map** (also called a dictionary or object).

## 5.2 What a Map Looks Like

```cco
/* NOTE: a map containing person information */
person: (
    name: "Alice",
    age: 30,
    city: "Beijing"
)
```

Outer parentheses `(...)` or braces `{...}` enclose comma-separated key-value pairs.

## 5.3 Empty Map

An empty `()` or `{}` is an empty map:

```cco
empty_object: ()
```

## 5.4 Nested Maps

A map's values can themselves be maps:

```cco
config: (
    database: (
        host: "localhost",
        port: 5432
    ),
    cache: (
        host: "redis.local",
        port: 6379
    )
)
```

## 5.5 Values Can Be Any Type

Each value in a map can be:
- A primitive (number, string, bool, None)
- Another map
- An array (next chapter)
- A template instance (covered later)

```cco
mixed: (
    a: 42,                    /* NOTE: integer */
    b: "hello",               /* NOTE: string */
    c: true,                  /* NOTE: boolean */
    d: None,                  /* NOTE: null */
    e: (nested: "map"),       /* NOTE: nested map */
    f: 3.14                   /* NOTE: float */
)
```

## 5.6 Common Mistakes

```cco
/* NOTE: missing comma between pairs -- ERROR */
(name: "Alice" age: 30)

/* NOTE: missing key name -- ERROR */
(: 42)

/* NOTE: using = instead of : -- ERROR */
(name = "Alice")
```

```cco
/* NOTE: correct */
(name: "Alice", age: 30)
```

## 5.7 Keys Can Be Identifiers or Strings

Map keys can be written as identifiers (letters, digits, underscores; must not start with a digit) without quotes, or as quoted strings.

```cco
/* NOTE: valid keys */
(name: 1, _count: 2, "content-type": 3)
```

```cco
/* NOTE: valid keys */
(name: 1, _count: 2, myField: 3)

/* NOTE: invalid -- starts with a digit */
(1st: 1)
```

## 5.8 Summary

- Maps use `(key: value, ...)` syntax
- Empty map is `()`
- Values can be any type, maps can nest
- Keys are identifiers, no quotes needed

Next chapter -- arrays.

Trailing commas are allowed: `(a: 1, b: 2,)`.


<!-- 06-arrays/readme.md -->
# Chapter 6: Arrays -- Lists

After this chapter you will be able to write arrays and understand the difference between maps and arrays.

## 6.1 Arrays Use `()` or `[]`

CCO arrays can use traditional parentheses or standard brackets:

```cco
/* NOTE: an array of numbers */
numbers: [1, 2, 3, 4, 5]
```

```cco
/* NOTE: an array of numbers */
numbers: (1, 2, 3, 4, 5)
```

## 6.2 How to Tell Arrays from Maps?

Here is the rule you need to remember:

> If you use `[]` it is always an array. If you use `{}` it is always a map. If you use `()`, and the **first element** is a `key: value` pair, it is a map. Otherwise it is an array.

| Writing | First Element | Result |
|---------|--------------|--------|
| `(x: 1, y: 2)` | `x: 1` (has colon) | map |
| `(42, "hi")` | `42` (no colon) | array |
| `()` | empty | map (default) |

## 6.3 Array Examples

```cco
empty: (),                          /* NOTE: empty -- defaults to map */
nums: (1, 2, 3),                   /* NOTE: numeric array */
mixed: (42, "hello", true, None),  /* NOTE: mixed-type array */
nested: (
    (1, 2),
    (3, 4)                         /* NOTE: 2D array */
)
```

## 6.4 Pitfalls

You might think you wrote an array, but CCO sees a map:

```cco
/* NOTE: looks like a single-element array? */
(x: 1)

/* NOTE: actually a map { x: 1 } */
```

If the first element is `ident: value`, it is treated as a map. For a single-element array, make sure the first element is **not** an `ident: value` form:

```cco
/* NOTE: single-element array */
(42)

/* NOTE: single-element array (string, not identifier) */
("hello")
```

## 6.5 Practical Examples

```cco
users: (
    "Alice",
    "Bob",
    "Charlie"
)

scores: (
    (name: "Alice", score: 95),
    (name: "Bob", score: 87),
    (name: "Charlie", score: 92)
)
```

Above, `scores` is an array where each element is a map.

## 6.7 Ranges

Generate arrays up to 10000 elements dynamically using `..` (half-open) or `..=` (inclusive):

```cco
[1..10]    /* 1 to 9 */
[1..=10]   /* 1 to 10 */
[10..1]    /* 10 down to 2 */
```

## 6.8 Summary

| | Map | Array |
|---|-----|-------|
| Syntax | `(k: v, ...)` | `(v, v, ...)` |
| Elements | key-value pairs | values |
| Empty `()` | default | -- |
| First element `k: v` | yes | no |

Next chapter -- the top-level shorthand, the most common way to write `.cco` files.

Trailing commas are allowed: `(1, 2, 3,)` / `[1, 2, 3,]`.


<!-- 07-shorthand/readme.md -->
# Chapter 7: Top-Level Shorthand

After this chapter you will understand the overall structure of a `.cco` file.

## 7.1 Review

In Chapter 2 we wrote:

```cco
name: "libcco",
version: 1
```

In Chapter 5 we said this is equivalent to a map. Strictly speaking, at the **file top level**, writing `key: value` is a **shorthand** for `(key: value)`.

## 7.2 Full Form vs Shorthand

These two are equivalent:

```cco
/* NOTE: shorthand at top level (recommended for daily use) */
name: "libcco",
version: 1

/* NOTE: full form (same result) */
(name: "libcco", version: 1)
```

**Most people use the shorthand** -- it is cleaner and more natural.

## 7.3 When Must You Use `()` ?

**The shorthand only works at the outermost file level.** Inside a nested map, you must write `(k: v, ...)`:

```cco
/* NOTE: correct */
app: (                          /* NOTE: app: is top-level shorthand */
    name: "libcco",              /* NOTE: inside a map now */
    version: 1
)

/* NOTE: ERROR -- cannot use shorthand inside a map */
app:
    name: "libcco",
    version: 1
```

## 7.4 Complete Example

```cco
/* NOTE: a user config using top-level shorthand */
name: "Alice",
age: 30,
address: (
    city: "Beijing",
    zip: "100000"
),
hobbies: (
    "reading",
    "running",
    "coding"
)
```

This file is a map at the top level, equivalent to:

```cco
(
    name: "Alice",
    age: 30,
    address: (city: "Beijing", zip: "100000"),
    hobbies: ("reading", "running", "coding")
)
```

## 7.5 Summary

- `.cco` files can omit the outer `(...)` at the top level and write `key: value` directly
- Multiple pairs are separated by commas
- Inside nested structures, you **must** use `(...)`

Now that you understand CCO's data model, the next chapters introduce the type system.

## 7.6 Exercise

Try creating a `profile.cco` file with: name, age, address (city + zip), and hobbies. Compare with `/learn/source/basic.cco`.


<!-- 08-type-basics/readme.md -->
# Chapter 8: Type Basics

After this chapter you will understand the concept of "types" in CCO -- essential for `$typedef`, `$enum`, and `$temp`.

## 8.1 What Is a Type?

"Type" means "kind of data". Each value from Chapter 3 has a corresponding type:

| Value Example | Type Name | Purpose |
|--------------|-----------|---------|
| `"hello"` | `String` | text |
| `42` | `Integer` | whole numbers |
| `3.14` | `Float` | decimal numbers |
| `true` / `false` | `Boolean` | true/false |
| `None` | `None` | null/empty |

In later chapters, we use these type names to declare "what type this field should be".

## 8.2 Why Do Types Matter?

Suppose you are configuring a database. You want:

- `host` to be text (String)
- `port` to be a number (Integer)

With a type system, you can **explicitly declare** each field's type, reducing the chance of mistakes. You will see this in action when we cover `$temp`.

## 8.3 These Five Keywords Are Reserved

In CCO, these five words are reserved:

```
String    Integer    Float    Boolean    None
```

They cannot be used as variable names or field names.

## 8.4 Bonus: Array and dyn

Besides the five primitives, CCO also supports:

- `Array` -- array type (for constraining array element types)
- `dyn BaseTemplate` -- dynamic type, meaning "any instance of BaseTemplate or its subclasses"

You just need to know these exist for now. Details come later.

## 8.5 Summary

| Keyword | Meaning |
|---------|---------|
| `String` | text |
| `Integer` | integer |
| `Float` | floating-point number |
| `Boolean` | true/false |
| `None` | null |
| `Array` | array |
| `dyn` | dynamic dispatch (polymorphism) |

Next chapter -- type aliases with `$typedef`.


<!-- 09-typedef/readme.md -->
# Chapter 9: `$typedef` -- Type Aliases

After this chapter you will be able to give meaningful names to types using `$typedef`.

## 9.1 The URL Problem

Suppose you write an API configuration:

```cco
api: (
    url: "https://example.com"
)
```

The `url` value is a string, but "URL" and "plain string" are semantically different. If you could define `url`'s type as `Url`, the config becomes clearer.

## 9.2 Defining an Alias with $typedef

```cco
$typedef.Url: String,
```

This means: **`Url` is an alias for `String`** .

Now use it:

```cco
api: (
    url: "https://example.com"
)
```

> Note: `$typedef` does not enforce type constraints at parse time -- it serves as documentation and semantic annotation.

## 9.3 Syntax Breakdown

```
$typedef .   Name   :   Type   ,
   ^      ^    ^      ^    ^
keyword   dot  name  colon  type  comma (separator)
```

- `$typedef` is the keyword
- `.` the dot (required)
- `Name` is your alias (conventionally capitalized)
- `:` colon
- `Type` can be `String`, `Integer`, `Float`, `Boolean`, `None`, or another alias

## 9.4 Multiple Aliases

```cco
$typedef.Url: String,
$typedef.Port: Integer,
$typedef.Timeout: Integer,

app: (
    url: "https://example.com",
    port: 8080,
    timeout: 30
)
```

## 9.5 Placement Rules

All `$typedef` declarations must appear at the **file top level**, before the root value:

```cco
/* NOTE: correct -- declarations first, data after */
$typedef.Url: String,
app: (url: "https://example.com")
```

```cco
/* NOTE: ERROR -- $typedef cannot appear inside a map */
app: (
    $typedef.Url: String,
    url: "https://example.com"
)
```

## 9.6 Notes

- `$typedef` declarations are separated by commas
- Alias names must not conflict with keywords or existing names
- Aliases can reference other aliases: `$typedef.SecureUrl: Url`

## 9.7 Summary

- `$typedef.Name: Type` creates a type alias
- Must appear at file top level
- Makes config semantics clearer

Next chapter -- enumerations.


<!-- 10-enum/readme.md -->
# Chapter 10: `$enum` -- Enumerations

After this chapter you will be able to define and use enumerations to constrain allowed values.

## 10.1 The Mode Problem

Suppose you have a server config with a "mode" -- development, staging, or production. Without enums, you might write raw strings:

```cco
server: (
    host: "localhost",
    mode: "dev"          /* NOTE: "dev", "test", "prod" -- but what if someone misspells? */
)
```

The problem: someone might write `"DEV"` or `"dev "` with an extra space. **Enumerations** solve this.

## 10.2 Defining an Enum with $enum

```cco
$enum.Mode = (DEV, STAGING, PROD),
```

Then use it:

```cco
server: (
    host: "localhost",
    mode: PROD           /* NOTE: only DEV, STAGING, or PROD allowed */
)
```

## 10.3 Syntax Breakdown

```
$enum  .  Name  =  (  VAL1  ,  VAL2  ,  ...  )  ,
  ^     ^  ^    ^    ^      ^       ^         ^
keyword dot name equal  left-paren  comma-sep  right-paren  comma
```

- Values are separated by commas
- Enum values are plain identifiers (no quotes)

## 10.4 Enum Values Are Integers Internally

Enum values are stored as integers, numbered from 0 in definition order:

```cco
$enum.Color = (RED, GREEN, BLUE),
/* RED = 0, GREEN = 1, BLUE = 2 */
```

So `mode: PROD` stores as the number `2`. But you **write the name**, not the number.

## 10.5 Multiple Enums

```cco
$enum.Mode = (DEV, STAGING, PROD),
$enum.Level = (LOW, MEDIUM, HIGH),
$enum.Color = (RED, GREEN, BLUE),

app: (
    mode: DEV,
    level: HIGH,
    color: GREEN
)
```

## 10.6 Enum vs String

```cco
/* NOTE: error-prone */
mode: "dev",

/* NOTE: parser catches typos */
mode: DEV
```

If you write `DEVZ` instead of `DEV`, the parser immediately reports an error -- no silent bugs.

## 10.7 Summary

- `$enum.Name = (VAL1, VAL2, ...)` defines an enumeration
- Enum values conventionally use UPPERCASE (not enforced)
- Values are stored as integers (starting at 0)
- Misspelled enum values produce parse errors, preventing hidden bugs

Next chapter -- templates, CCO's most powerful feature.


<!-- 11-template/readme.md -->
# Chapter 11: What is a Template?

After this chapter you will understand what CCO "templates" are and what problem they solve.

## 11.1 The Repetition Problem

Suppose you have many servers to configure:

```cco
server1: (
    host: "web1.example.com",
    port: 8080
),
server2: (
    host: "web2.example.com",
    port: 8080
),
server3: (
    host: "web3.example.com",
    port: 8080
)
```

Now you need to change `port` to 9090 -- you have to change it in three places. What if there were 30 servers?

## 11.2 Template = Blueprint

A template (`$temp`) acts as a **blueprint** defining what fields a "server" has. When you need a server, you instantiate the blueprint with specific values:

```cco
$temp.Server: (
    host: String,
    port: Integer
),

server1: #Server("web1.example.com", 8080),
server2: #Server("web2.example.com", 9090)
```

- `$temp.Server` defines a template called `Server`
- `#Server(...)` means "create an instance from this template"
- To change the port, you only need to change `$temp.Server`'s definition

## 11.3 Comparison with Programming

| Concept | Analogy |
|---------|---------|
| Template `$temp` | class, struct |
| Instance `#Server(...)` | object, instance |
| Field `host: String` | property, member variable |

## 11.4 Summary

- Templates solve the "repeating the same structure" problem
- `$temp.Name: (...)` defines a template
- `#Name(...)` creates an instance of a template

Next chapter -- template fields and default values.


<!-- 12-template-fields/readme.md -->
# Chapter 12: Fields and Default Values

After this chapter you will be able to write template fields with type annotations and default values.

## 12.1 The Simplest Template

```cco
$temp.Point: (
    x: Integer,
    y: Integer
)
```

Defines a "point" template with two integer fields: `x` and `y`.

## 12.2 Full Field Syntax

```
field_name  :   Type   =   Default_Value
    ^         ^    ^         ^
identifier  colon type      equal + value (optional)
```

Both type and default value are optional:

```cco
$temp.Example: (
    a: String,        /* NOTE: has type, no default */
    b: Integer = 42,  /* NOTE: has type, has default */
    c: "hi"           /* NOTE: no type, has default (type inferred) */
)
```

## 12.3 Using Default Values

```cco
$temp.Server: (
    host: String,
    port: Integer = 8080,
    timeout: Integer = 30
),

/* Only pass required args */
s1: #Server("localhost"),              /* NOTE: port=8080, timeout=30 */
s2: #Server("example.com", 443),       /* NOTE: timeout=30 */
s3: #Server("test.com", 80, 60)        /* NOTE: all specified */
```

Fields with defaults can be omitted when instantiating.

## 12.4 Acceptable Types for Fields

A field's type can be:

- Primitive: `String`, `Integer`, `Float`, `Boolean`, `None`
- Enum name: `Mode`, `Color`, etc.
- Another template name: `Server`, `Endpoint`, etc.
- An alias: any name from `$typedef`

```cco
$typedef.Url: String,
$enum.Mode = (DEV, PROD),

$temp.Endpoint: (
    url: Url,
    mode: Mode
)
```

## 12.5 Summary

- Field format: `name: Type = Default`
- Type is optional but recommended
- Default values are optional but convenient
- Types can be primitives, enums, aliases, or templates

Next chapter -- instantiating templates.


<!-- 13-instantiation/readme.md -->
# Chapter 13: Instantiation -- Positional Arguments

After this chapter you will be able to create template instances by passing positional arguments.

## 13.1 What is Instantiation?

After defining a template, you use `#TemplateName(...)` to create an **instance** of it.

## 13.2 Positional Arguments

The simplest way: pass values in field order.

```cco
$temp.Point: (x: Integer, y: Integer),

origin: #Point(0, 0),         /* NOTE: x=0, y=0 */
p1:     #Point(3, 5)          /* NOTE: x=3, y=5 */
```

`#Point(0, 0)` -- first value `0` goes to first field `x`, second value `0` to second field `y`. This is **positional passing**.

## 13.3 With Default Values

```cco
$temp.Server: (
    host: String,
    port: Integer = 8080,
    timeout: Integer = 30
),

s1: #Server("localhost"),           /* NOTE: only host; port and timeout use defaults */
s2: #Server("example.com", 443),    /* NOTE: specify host and port; timeout default */
s3: #Server("test.com", 80, 60)     /* NOTE: all specified */
```

Fields with defaults can be omitted, as long as all preceding required fields are supplied.

## 13.4 Nested Instantiation

```cco
$temp.Endpoint: (
    name: String,
    url: String
),

$temp.App: (
    endpoints: Array
),

app: #App((
    #Endpoint("api", "https://api.example.com"),
    #Endpoint("admin", "https://admin.example.com")
))
```

## 13.5 Summary

- `#TemplateName(val1, val2, ...)` passes args positionally
- Fields with defaults can be omitted
- Instances can nest

Next chapter -- named instantiation (passing args by name).


<!-- 14-named-inst/readme.md -->
# Chapter 14: Named Instantiation

After this chapter you will be able to instantiate templates using named parameters -- no need to remember field order.

## 14.1 The Pain of Positional Args

Consider this template:

```cco
$temp.Server: (
    host: String,
    port: Integer = 8080,
    timeout: Integer = 30,
    retries: Integer = 3
)
```

If you only want to set `retries = 5`, positional args force you to write everything:

```cco
s1: #Server("localhost", 8080, 30, 5)  /* NOTE: must write all 4 args */
```

## 14.2 Named Args to the Rescue!

Named args let you **specify only the fields you want**:

```cco
s1: #Server(.host: "localhost", .retries: 5),
s2: #Server(.host: "example.com", .port: 443, .retries: 10)
```

Syntax: `.field_name: value`

## 14.3 Rules

- Named args are matched **by name**, not by position
- Unspecified fields use their defaults
- Each field name must be prefixed with a dot `.`

```cco
/* NOTE: order does not matter */
s1: #Server(.retries: 5, .host: "localhost", .timeout: 60)

/* NOTE: same result */
s2: #Server(.host: "localhost", .timeout: 60, .retries: 5)
```

## 14.4 Positional vs Named

| | Positional | Named |
|---|-----------|-------|
| Syntax | `#T(v1, v2)` | `#T(.f1: v1, .f2: v2)` |
| Order | must match field order | any order |
| Readability | count positions | self-documenting |
| Skipping defaults | awkward | just omit |

For **more than 3 fields**, named instantiation is strongly recommended.

## 14.5 Summary

- `#Template(.field: value, ...)` -- named instantiation
- Unspecified fields use defaults
- Order is arbitrary, better readability

Next chapter -- template inheritance.


<!-- 15-inheritance/readme.md -->
# Chapter 15: Template Inheritance

After this chapter you will be able to extend existing templates using inheritance.

## 15.1 The Problem

You have a basic server template:

```cco
$temp.Server: (
    host: String,
    port: Integer = 8080
)
```

Now you need a "secure server" that has everything `Server` has, plus a `cert` and `ssl_port`.

## 15.2 Inheritance with `+ Parent`

```cco
$temp.SecureServer + Server: (
    cert: String,
    ssl_port: Integer = 443
)
```

Syntax: `$temp.ChildName + ParentName: ( ... )`

`SecureServer` automatically has all of `Server`'s fields (`host`, `port`), plus its own new fields (`cert`, `ssl_port`).

## 15.3 Using an Inherited Template

```cco
/* NOTE: positional args -- parent fields first, child fields after */
s1: #SecureServer("localhost", 8080, "/etc/cert.pem", 443),

/* NOTE: named args (recommended) */
s2: #SecureServer(.host: "api.example.com", .cert: "/etc/cert.pem", .ssl_port: 8443)
```

With positional args, **parent fields come first**, then child fields.

## 15.4 Multiple Inheritance?

CCO supports **single inheritance only** -- one parent per template. But chains can be long:

```cco
$temp.Shape: ( area: Float ),
$temp.Circle + Shape: ( radius: Float ),
$temp.SmallCircle + Circle: ( label: String ),

/* SmallCircle has area + radius + label */
c: #SmallCircle(3.14, 1.5, "tiny")
```

## 15.5 Common Use Cases

1. **Base + extension**: `Server` -> `SecureServer`
2. **Abstract + concrete**: `Animal` -> `Dog`, `Cat`
3. **Layered configs**: `BaseConfig` -> `DatabaseConfig`, `CacheConfig`

```cco
$temp.BaseEndpoint: (
    url: String,
    timeout: Integer = 30
),

$temp.RESTEndpoint + BaseEndpoint: (
    method: String = "GET"
),

$temp.GraphQLEndpoint + BaseEndpoint: (
    schema: String
)
```

## 15.6 Summary

- `$temp.Child + Parent: (...)` creates an inherited template
- Child has all parent fields
- Single inheritance only (but chains can be long)
- Named instantiation is especially useful with inheritance

Congratulations! You have learned CCO's core features. Next chapter -- expressions.


<!-- 16-expressions/readme.md -->
# Chapter 16: Expressions

After this chapter you will be able to use arithmetic operators in CCO expressions.

## 16.1 What Is an Expression?

So far, all values have been fixed literals:

```cco
count: 42
```

But what if `count` needs to be computed from other values? CCO's expression system handles this.

## 16.2 Writing Expressions with `$(...)`

```cco
result: $(10 + 20)
```

`$(...)` is the expression wrapper. Inside, you can use various operators, and CCO will evaluate them. The above yields `30`.

## 16.3 Arithmetic Operators

| Operator | Meaning | Example | Result |
|----------|---------|---------|--------|
| `+` | addition | `$(10 + 20)` | 30 |
| `-` | subtraction | `$(10 - 3)` | 7 |
| `*` | multiplication | `$(6 * 7)` | 42 |
| `/` | division | `$(10 / 3)` | 3 (integer) |

## 16.4 Power and Bitwise Operators

CCO also supports the power operator `**` and bitwise operators `|`, `&`, `^`, `~`:

```cco
a: $(2 ** 3),      /* NOTE: 8 */
b: $(3 | 5),       /* NOTE: bitwise OR, 7 */
c: $(~0)           /* NOTE: bitwise NOT */
```

## 16.5 Operator Precedence

Expressions are evaluated strictly according to the following precedence levels (from highest to lowest):

1. **Unary**: `+` `-` `!` `~`
2. **Power**: `**` (Right-associative)
3. **Multiplication/Division**: `*` `/`
4. **Addition/Subtraction**: `+` `-`
5. **Ranges**: `..` `..=`
6. **Coalesce**: `??`
7. **Comparison**: `<` `<=` `>` `>=`
8. **Equality**: `==` `!=`
9. **Bitwise AND**: `&`
10. **Bitwise XOR**: `^`
11. **Bitwise OR**: `|`
12. **Logical AND**: `&&`
13. **Ternary**: `? :` (Right-associative)
14. **Logical OR**: `||`

You can always use nested `$(...)` to force a specific evaluation order.

If either operand is float, the result is float.

```cco
A: $(10 + 20 * 3)    /* NOTE: = 10 + 60 = 70 */
B: $($(10 + 20) * 3) /* NOTE: = 30 * 3 = 90 */
```

## 16.6 Where Can Expressions Be Used?

Expressions mainly appear in:
- Template field default values
- Constructor method bodies (Chapter 19)
- Static method bodies (Chapter 20)

For evaluation, the C API (`cco_expr_eval`) is used.

## 16.7 Summary

- `$(expression)` marks an expression
- Supports `+` `-` `*` `/` `**` `|` `&` `^` `~`
- Multiplication/division before addition/subtraction
- Nested `$(...)` changes precedence

Next chapter -- comparison and logical operators.



<!-- 17-comparison-logic/readme.md -->
# Chapter 17: Comparison & Logic

After this chapter you will be able to use comparison and logical operators in CCO expressions.

## 17.1 Comparison Operators

| Operator | Meaning | Example |
|----------|---------|---------|
| `==` | equal | `$(10 == 10)` -> `true` |
| `!=` | not equal | `$(10 != 5)` -> `true` |
| `<` | less than | `$(3 < 5)` -> `true` |
| `>` | greater than | `$(10 > 5)` -> `true` |
| `<=` | less or equal | `$(5 <= 5)` -> `true` |
| `>=` | greater or equal | `$(5 >= 3)` -> `true` |

Results are boolean values `true` or `false`:

```cco
a: $(10 == 10),   /* NOTE: true */
b: $(10 != 5),    /* NOTE: true */
c: $(3 > 10)      /* NOTE: false */

both:  $(true && true),       /* NOTE: true */
either: $(true || false),     /* NOTE: true */
not_it: $(!false)             /* NOTE: true */

result: $($(10 > 5) && $(3 < 10)),   /* NOTE: true && true = true */
result: $($(10 == 10) || $(5 == 3))  /* NOTE: true || false = true */
```

## 17.4 Common Mistakes

```cco
/* NOTE: == for equality, not = */
a: $(10 = 10)    /* ERROR */

/* NOTE: string comparison with operators is limited */
a: $("hello" == "world")   /* may not work as expected */
```

> Current CCO comparison operators mainly support numeric types (Integer and Float).

## 17.5 Ternary Operator

You can also use the ternary operator `? :` for simple conditional logic:

```cco
status: $(is_active ? "online" : "offline")
```

## 17.6 Summary

- Comparison: `==` `!=` `<` `>` `<=` `>=`
- Logical: `&&` `||` `!`
- Ternary Operator: `? :`
- Results are always boolean

Next chapter -- the coalescing operator.


<!-- 18-coalesce/readme.md -->
# Chapter 18: Coalescing Operator

After this chapter you will be able to handle "use default if None" scenarios gracefully.

## 18.1 The Problem

You have a config value that might be `None` (empty). You want: if it has a value, use it; if it's `None`, use a fallback.

## 18.2 The Coalescing Operator `??`

CCO uses the **double question mark `??`** as the coalescing operator:

```
left ?? right
```

**If the left value is not `None`, take the left; otherwise take the right.**

## 18.3 Examples

```cco
a: $(None ?? 42),    /* NOTE: None is discarded, result is 42 */
b: $(10 ?? 99),      /* NOTE: 10 is not None, result is 10 */
c: $(None ?? None ?? 5)  /* NOTE: chain: result is 5 */

port: Integer = None,    /* NOTE: default is None */

/* NOTE: Inside a constructor: */
/* NOTE: port ?? fallback_port -> use port if set, otherwise 8080 */

a: $(true || false),    /* NOTE: -> true (boolean) */
b: $(None ?? 42)         /* NOTE: -> 42 (integer) */
```

## 18.4 Summary

- `left ?? right`: if left is not None, use left; otherwise use right
- Can be chained: `$a ?? $b ?? $c`
- Different from `||` (logical OR)

Next chapter -- constructors.


<!-- 19-constructors/readme.md -->
# Chapter 19: Constructors `$function.@`

After this chapter you will be able to define custom constructors with initialization logic.

## 19.1 Why Constructors?

Currently, instantiation just assigns values to fields:

```cco
$temp.Point: (x: Integer, y: Integer),
origin: #Point(0, 0)
```

But what if initialization is more complex -- computing one field from another, or validation?

## 19.2 The Default Constructor

If no `$function.@` is defined, CCO auto-generates a **default constructor** that assigns positional args to fields in order. All previous `#Server(...)` examples used the default constructor.

Explicitly:

```cco
$temp.Server: (
    host: String,
    port: Integer = 8080
),
/* Same as the auto-generated default */
$function.@: $default
```

## 19.3 Custom Constructor

```cco
$temp.Server: (
    host: String,
    port: Integer = 8080,
    url: String = ""
),

$function.@<host: String, port: Integer>: (
    $this.(host: host, port: port, url: ("https://" + host + ":" + port))
)
```

Breaking it down:

- `$function.@<host: String, port: Integer>: (...)`
  - `$function.@` means "I am defining a constructor"
  - `<host: String, port: Integer>` is the parameter list
  - `:` then `(...)` is the method body

Inside the body, `$this.(...)` assigns to the instance's fields:

```cco
$this.(host: host, port: port, url: ...)
```

This means: set field `host` to parameter `host`, field `port` to parameter `port`, field `url` to the computed expression.

## 19.4 Parameter Names vs Field Names

Constructor parameters can have the **same name** as fields (most common):

```cco
$function.@<host: String, port: Integer>: (
    $this.(host: host, port: port)
    /* NOTE:       ^field   ^param */
)
```

Or different names:

```cco
$function.@<h: String, p: Integer>: (
    $this.(host: h, port: p)
)
```

## 19.5 Anonymous Parameters `_$0`, `_$1`

If parameter names are unimportant:

```cco
$function.@<_$0: String, _$1: Integer>: (
    $this.(host: _$0, port: _$1)
)
```

## 19.6 Multiple Constructors

A template can have multiple constructors. If you define at least one custom constructor, the default is **not** auto-generated.

```cco
$temp.Server: (
    host: String,
    port: Integer = 8080
),

/* Custom constructor: only accepts host */
$function.@<host: String>: (
    $this.(host: host, port: 8080)
)

/* NOTE: only the custom constructor is available */
s1: #Server("localhost")     /* OK */
/* s2: #Server("local", 443) /* ERROR -- no matching constructor */
```

## 19.7 Summary

- `$function.@<params>: (body)` defines a custom constructor
- Body uses `$this.(...)` to assign fields
- Custom constructors replace the default

Next chapter -- static methods.


<!-- 20-static-methods/readme.md -->
# Chapter 20: Static Methods `$function.#`

After this chapter you will be able to define and call static methods on templates.

## 20.1 What Is a Static Method?

A static method **belongs to the template itself**, not to any instance. You call it without creating an instance -- similar to class methods in other languages.

## 20.2 Defining a Static Method

```cco
$temp.Math: (),

$function.#square<x: Integer>: Integer (
    $return<Integer>(x * x)
)
```

Syntax breakdown:

```
$function .  # MethodName  <Params>  :  ReturnType  (  Body  )
```

- `$function.#square` -- defines a static method named `square`
- `<x: Integer>` -- parameter list (same as constructors)
- `: Integer` -- return type
- `(...)` -- method body

## 20.3 Calling Static Methods

```cco
    result: #Math:square(5)      /* NOTE: result is 25 */
    result: Math:square(6)       /* NOTE: alternative syntax */
```

Two calling styles:

| Style | Example |
|-------|---------|
| `#Template:method(args)` | `#Math:square(5)` |
| `Template:method(args)` | `Math:square(6)` |

## 20.4 Statements Inside Method Bodies

Method bodies support these statements:

| Statement | Example | Purpose |
|-----------|---------|---------|
| bind | `result: Integer = x * x` | declare a local variable |
| assign | `result = x * x` | assign to local variable |
| return | `$return<Integer>(result)` | return a value |
| expr | `callSomething()` | evaluate an expression (discard result) |

```cco
$function.#compute<a: Integer, b: Integer>: Integer (
    temp: Integer = a + b,
    $return<Integer>(temp * 2)
)
```

## 20.5 The Return Statement

`$return<Type>(expression)` returns a value:

```cco
$function.#max<a: Integer, b: Integer>: Integer (
    $return<Integer>($(a > b ? a : b))
)
```

## 20.6 Summary

- `$function.#Method<params>: ReturnType ( body )` defines a static method
- Called via `#Template:method(args)` or `Template:method(args)`
- Body supports bind, assign, return, and expression statements

Next chapter -- `$format`.


<!-- 21-format/readme.md -->
# Chapter 21: `$format` Function

After this chapter you will be able to parse strings as CCO values at runtime using `$format`.

## 21.1 The Problem

Sometimes you need to parse a text string as a CCO value at runtime. For example, a config contains the string `"(1, 2, 3)"` but you want it as an actual array.

## 21.2 Basic Usage

```cco
result: $format("(1, 2, 3)")
```

`$format(string)` parses the string as CCO code and returns the resulting value. The example above produces a three-element array.

## 21.3 Practical Example

```cco
/* NOTE: stored as a raw string */
raw: "(1, 2, 3)",

/* NOTE: parsed into a real array */
    parsed: $format(raw)     /* NOTE: result is an array containing 1, 2, 3 */
```

## 21.4 What Can Be Parsed?

`$format` can parse any valid CCO value expression:

- Primitives: `$format("42")` -> integer 42
- Strings: `$format('"hello"')` -> string "hello"
- Maps: `$format('(x: 1, y: 2)')` -> map
- Arrays: `$format("(1, 2, 3)")` -> array
- Template instances: `$format('#Point(0, 0)')` -> template instance

## 21.5 Notes

- The argument must be a string (or an expression that evaluates to a string)
- Parsing uses the current symbol table (so defined templates and enums are available)
- If the string is invalid CCO, an error is returned

## 21.6 Summary

- `$format(string)` parses CCO text at runtime
- Can parse any valid CCO value
- Useful for dynamic configuration

Next chapter -- appendix: complete grammar reference.


<!-- 22-grammar/readme.md -->
# Chapter 22: Complete Grammar Reference

> If you don't need to know the grammar definition, you can skip it.

This is the full CCO grammar specification in EBNF-like notation. Useful for quick reference.

## 22.1 File Structure

```
File       ::= (Declaration ",")* RootValue ","?
RootValue  ::= Map | Array | Literal | TemplateInstance
```

**Parsing note (root-less files):** `cco_parse_full()` requires a `RootValue`
(else: `"no root expression in .cco file"`). The multi-context entry points
`cco_parse_full_froms(text, ...)` and `cco_parse_full_from_files(path, ...)`
accept **declaration-only input**: with zero (or no) root values they return a
valid result whose `root` is `NULL` and whose symbol table holds every declared
type/function. A schema-only file such as `search/openamethyst.system.cco`
parses fine through these entry points (parser.c:1833-1861).

## 22.2 Values

```
Value      ::= Map | Array | Literal
             | TemplateInstance | StaticCall
             | EnumConstant | Identifier
             | FunctionCall

Map        ::= "(" Field ("," Field)* ","? ")"
             | Identifier ":" Value           (top-level shorthand, file root only)
Field      ::= Identifier ":" Value

Array      ::= "(" Value ("," Value)* ","? ")"
Empty "()" ::= treated as empty Map

FunctionCall ::= "$" Identifier "(" Args ")"   (value-layer call; builtin "env")
```

**Disambiguation**: If the first element of `(...)` is `Identifier : Value`, it is a Map; otherwise an Array.

## 22.3 Literals

```
Literal    ::= String | RawString | Integer | Float
             | "true" | "false" | "None"

String     ::= '"' (char | escape)* '"'
RawString  ::= '`' (any char)* '`'
Integer    ::= decimal | "0x" hex | "0o" octal | "0b" binary
               Underscores "_" allowed between digits
Float      ::= digits "." digits [exponent]
             | digits exponent
             | integer "." integer
             Underscores "_" allowed between digits
```

## 22.4 Declarations

```
Declaration ::= TypeAlias | Enum | Template | Function

TypeAlias  ::= "$typedef" "." Identifier ":" TypeExpr
Enum       ::= "$enum" "." Identifier "=" "(" Identifier ("," Identifier)* ","? ")"
Template   ::= "$temp" ["<" GenericParam ">"] "." Name ["+" ParentName] ":" "(" TemplateBodyItem* ")"

Function   ::= StandaloneFunction | AttachedConstructor

StandaloneFunction
           ::= "$function" "." ["!"] "#" FunctionName Signature [Body]
AttachedConstructor
           ::= "$function" "." "@" ["<" ParamList ">"] ":" "(" Body ")"
             | "$function" "." "@" ":" "$default"     (in-template only, see 22.5)
```

Template names are **dotted**: `$temp.Provider.OpenAI:` declares template
`Provider.OpenAI`; the parent is dotted too, `$temp.MyTool + Tool.Function:`
(parser.c:1526, 1534). Standalone functions use the `#` form with the same
signature sugar as methods (parser.c:1256-1290); a missing `Body` is allowed
(bodyless, e.g. `$function.#env: (name: String) -> String`). The top-level
constructor attach `$function.@<Params>: (Body)` attaches to the most recently
declared template (deferred via `flush_pending_ctors`, parser.c:1221-1253,
1178-1200).

## 22.5 Template Body

```
TemplateBodyItem ::= FieldDecl | Constructor | StaticMethod

FieldDecl  ::= Identifier ["<" Annotation ">"] [":" TypeExpr] ["=" DefaultValue]
Constructor ::= "$function" "." "@" ["<" ParamList ">"] ":" ( "(" Body ")" | "$default" )
StaticMethod ::= "$function" "." ["<" GenericParams ">"] ["!"] "#" MethodName
                 ["<" GenericArgs ">"] "(" ParamList ")" ":" ReturnType "(" Body ")"
```

In-template static methods accept the same signature sugar as standalone
functions (`Signature`, 22.6): `: (Params) -> Ret (Body)`,
`<Params> : Ret (Body)`, `(Params) : Ret (Body)`, or bodyless `: Ret`
(parser.c:1613-1625, via `parse_function_signature` at 1136-1166).

## 22.6 Parameters and Body

```
ParamList  ::= (Param ("," Param)*)?
Param      ::= Identifier ":" TypeExpr | "_$" digits ":" TypeExpr

Signature  ::= [ParamsThen] [":" TypeExpr | "->" TypeExpr]     (sugar, may be empty)
ParamsThen ::= "<" ParamList ">" | ":" "(" ParamList ")" | "(" ParamList ")"

Body       ::= (Stmt ("," Stmt)* ","?)? [Expr]
Stmt       ::= Bind | Assign | Return | ExprStmt
Bind       ::= Identifier ":" TypeExpr "=" Expr
Assign     ::= "$this" "." Identifier "=" Expr
Return     ::= "$return" "<" TypeExpr ">" "(" Expr ")"
```

## 22.7 Type Expressions

```
TypeExpr   ::= ( Primitive | ArrayType | NamedType | DynType
               | MapType | InlineObject ) ["?"]

Primitive  ::= "String" | "Integer" | "Float" | "Boolean" | "None"
ArrayType  ::= "Array" ["<" TypeExpr ">"]
MapType    ::= "Map"                         (open map value, e.g. "Map?")
NamedType  ::= Name                          (dotted $typedef / $enum / $temp)
DynType    ::= "dyn" Identifier
InlineObject ::= "(" FieldType ("," FieldType)* ","? ")"
FieldType  ::= Identifier ":" TypeExpr ["=" DefaultValue]
```

Additions:
- **`"?"` nullable suffix** — postfix on any type (parser.c:890-893):
  `Url?`, `Map?`, `(type: ProxyType, host: String, port: Integer)?`.
- **`Map` primitive** — open map type (parser.c:819), distinct from
  `( FieldList )` inline objects.
- **`Array`** — currently emitted by the parser as bare `Array` with a `None`
  element type (parser.c:820-823).
- **dotted `Name`** — named types reference dotted templates: `Content.List?`,
  `Agent.Topology`, `Message.List` (parser.c:824-829).
- **inline object `( FieldList )`** — an anonymous structural type with
  per-field defaults (parser.c:830-882; `= DefaultValue` at 854-860):
  `timeout: (connect: Integer = 10, transfer: Integer = 60)`.

## 22.8 Expressions

```
Expr       ::= LiteralExpr | ArrayLiteral | MapLiteral | Identifier | "$this"
             | "(" Expr ")"               (compound $)
             | OpExpr
             | CallExpr

OpExpr     ::= Expr BinaryOp Expr
             | UnaryOp Expr
             | Expr "??" Expr             (coalesce)
             | Expr "?" Expr ":" Expr     (ternary)

BinaryOp   ::= "+" | "-" | "*" | "/" | "**"
             | "==" | "!=" | "<" | ">" | "<=" | ">="
             | "&&" | "||"
             | "|" | "&" | "^"

UnaryOp    ::= "!" | "~"

CallExpr   ::= "$format" "(" Expr ")"
             | "$include" "(" StringLiteral ")"                     (merge)
             | "#" Identifier "(" Args ")"                           (instantiate)
             | "#" Identifier "(." Field ":" Value ("," Field ":")* ")"  (named inst)
             | "#" Identifier ":" Identifier "(" Args ")"           (static call via #)
             | Identifier "." Identifier "(" Args ")"               (static call via dot)
             | "#" Name ":" "(" ( FieldList | Args ) ")"            (colon instantiation)
             | Name "." Identifier "(" Args ")"                     (static call via dot, dotted tname)
             | "$" Identifier "(" Args ")"                          (value-layer function call)
```

New forms (todo 5 parser, parser.c):
- **Colon-instantiation `#Name: ( ... )`** — named (`#Gateway: (servers: (...), ...)`)
  or positional (`#Tool.Function: ("shell", "Execute shell command", "{}")`);
  dotted `Name` allowed; named fields may carry a leading `.` sugar
  (parser.c:521-588 named, 590-599 positional).
- **Static call via `#`** with dotted name: `#T:method(args)` — parser.c:602-647.
- **Static call via dot** with dotted template name: `A.B.method(args)` and the
  plain `T.method(args)` form — parser.c:727-776.
- **Value-layer function call `$name(args)`** — resolves a `$function.#name`
  declaration or the builtin `env` (parser.c:713-725; `env` eval at
  expr.c:1092-1103: `$env("VAR")` returns the variable value, or `None` if unset).

## 22.9 Comments and Whitespace

```
Comment ::= "/*" (any char)* "*/"
Whitespace ::= space | tab | newline | carriage return
```

## 22.10 Lexical Elements

```
Identifier   ::= (letter | "_") (letter | digit | "_")*
Keywords     ::= "true" | "false" | "None"
               | "String" | "Integer" | "Float" | "Boolean"
               | "Array" | "Map" | "dyn" | "this"
$-Keywords   ::= "$" letter+
                 Recognized: "$typedef" "$enum" "$temp" "$function"
                             "$default" "$this" "$return" "$format"
                 Any other "$name" (e.g. "$env") lexes as a "$"-prefixed
                 identifier and is used as a value-layer function call (22.8).
```

`?` is a single delimiter token (`CCO_TK_QMARK`) used as the nullable type
suffix (22.7); `.` is the dotted-name separator (22.4, 22.7, 22.8).

## 22.11 QA Checklist — new syntax vs parser.c

| Syntax | EBNF addition | parser.c line | search-file evidence |
|--------|---------------|---------------|----------------------|
| `?` nullable type suffix | 22.7 `TypeExpr ... ["?"]` | 890-893 | `system.cco:24` `region: String?`; `system.cco:31` `key: String?` |
| `Map` type keyword | 22.7 `MapType ::= "Map"` | 819 | `system.cco:45` `meta: Map?` |
| `Array` type | 22.7 `ArrayType` (existing) | 820-823 | `system.cco:67` `servers: Array`; `system.cco:35` `status_codes: Array = (429, 502, 503)` |
| dotted `Name` in `TypeExpr` | 22.7 `NamedType ::= Name` | 824-829 | `system.cco:44` `provider: Provider.Vendor?`; `system.cco:79` `tuning: Tuning.Params?` |
| inline object type `( FieldList )` + per-field defaults | 22.7 `InlineObject` / `FieldType` | 830-882 (defaults 854-860) | `system.cco:32` `timeout: (connect: Integer = 10, transfer: Integer = 60)`; `system.cco:33-35` nested `retry`/`backoff` |
| dotted template decl names | 22.4 `Template ::= ... Name ...` | 1526 | `system.cco:22` `$temp.Provider.Vendor:`; `system.cco:52` `$temp.Hook.Function:` |
| dotted template parent `+ Name` | 22.4 `Template ::= ... ["+" ParentName]` | 1534 | `request.cco:14` `$temp.CdnHook + Hook.Function:` |
| `$function.#name: (Params) -> Ret [Body]` | 22.4 `StandaloneFunction` + 22.6 `Signature` | 1256-1290, 1136-1166 | `system.cco:83` `$function.#env: (name: String) -> String` |
| `$function.#name<Params>: Ret (Body)` | 22.4 `StandaloneFunction` + 22.6 `Signature` | 1139, 1141-1148, 1157 | learn/20 `$function.#square<x: Integer>: Integer (...)` |
| top-level `$function.@<Params>: (Body)` constructor attach | 22.4 `AttachedConstructor` | 1221-1253 (flush 1178-1200) | learn/19 `$function.@<host: String, port: Integer>: (...)` |
| in-template static methods | 22.5 `StaticMethod` (existing) + `Signature` sugar | 1582-1630, 1613-1625 | learn/20 `$function.#square<x: Integer>: Integer (...)` |
| static call `#T:method(args)` | 22.8 `CallExpr` `# Name ":" Identifier "(" Args ")"` | 602-647 | learn/20 `#Math:square(5)` |
| static call `T.method(args)` | 22.8 `CallExpr` dot forms | 727-776 | learn/20 `Math:square(6)` |
| colon-instantiation `#Name: ( ... )` | 22.8 `CallExpr` `# Name ":" "(" ... ")"` | 521-588 (named), 590-599 (positional) | `request.cco:18` `gateway: #LoadBalancer: (`; `request.cco:72` `#CdnHook: ("purge", ...)` |
| value-layer `$name(args)` call | 22.2 `FunctionCall` + 22.8 | 713-725 | `request.cco:25` `key: $env("CLUSTER_TOKEN")` |
| `env` builtin | 22.2 `FunctionCall` note | expr.c 1092-1103 | `request.cco:25,39` `$env(...)` set/unset → value/None |
| root-less files | 22.1 parsing note | 1833-1861 (`parse_merged_text`), 1917 (`cco_parse_full_froms`) | `system.cco` (schema-only, parsed via from_files) |


<!-- 23-keywords/readme.md -->
# Chapter 23: Keywords & Operator Precedence

## 23.1 Complete Keyword List

### Primitive Type Keywords

| Keyword | Meaning |
|---------|---------|
| `String` | string type |
| `Integer` | integer type |
| `Float` | float type |
| `Boolean` | boolean type |
| `None` | none/null type |
| `Array` | array type |
| `dyn` | dynamic type (polymorphism) |

### Literal Keywords

| Keyword | Meaning |
|---------|---------|
| `true` | boolean true |
| `false` | boolean false |
| `None` | null value |

### Dollar Keywords

| Keyword | Chapter | Purpose |
|---------|---------|---------|
| `$typedef` | 9 | define type alias |
| `$enum` | 10 | define enumeration |
| `$temp` | 11 | define template |
| `$function` | 19-20 | define constructor or static method |
| `$default` | 19 | mark default constructor |
| `$this` | 19 | reference current instance |
| `$return` | 20 | return from method |
| `$format` | 21 | parse string as CCO at runtime |

### Other Keywords

| Keyword | Meaning |
|---------|---------|
| `this` | current instance reference (expression context) |

## 23.2 Operator Precedence (High to Low)

| Precedence | Operators | Associativity | Description |
|-----------|-----------|---------------|-------------|
| 1 (highest) | `!` | right | logical NOT (unary) |
| 2 | `*` `/` | left | multiply, divide |
| 3 | `+` `-` | left | add, subtract |
| 4 | `==` `!=` `<` `>` `<=` `>=` | left | comparison |
| 5 | `&&` | left | logical AND |
| 6 | `\|\|` | left | logical OR |
| 7 (lowest) | `\|` | left | coalesce |

## 23.3 Operator Quick Reference

| Operator | Prec | Meaning | Input Types | Result Type |
|----------|------|---------|-------------|-------------|
| `!x` | 1 | logical NOT | Boolean | Boolean |
| `a * b` | 2 | multiplication | Number | Number |
| `a / b` | 2 | division (integer truncation) | Number | Number |
| `a + b` | 3 | addition | Number | Number |
| `a - b` | 3 | subtraction | Number | Number |
| `a == b` | 4 | equal | Number | Boolean |
| `a != b` | 4 | not equal | Number | Boolean |
| `a < b` | 4 | less than | Number | Boolean |
| `a > b` | 4 | greater than | Number | Boolean |
| `a <= b` | 4 | less or equal | Number | Boolean |
| `a >= b` | 4 | greater or equal | Number | Boolean |
| `a && b` | 5 | logical AND | Boolean | Boolean |
| `a \|\| b` | 6 | logical OR | Boolean | Boolean |
| `a \| b` | 7 | coalesce | Any | Any |

## 23.4 Special Symbols

| Symbol | Name | Purpose |
|--------|------|---------|
| `(...)` | parentheses | map / array / grouping |
| `:` | colon | key-value separator / type annotation |
| `,` | comma | element separator |
| `.` | dot | separator in declarations |
| `+` | plus | addition / template inheritance |
| `#` | hash | template instantiation |
| `"..."` | double quote | string literal |
| `` `...` `` | backtick | raw string literal |
| `$` | dollar | keyword prefix |
| `@` | at | constructor marker |
| `/* */` | comment | block comment |
| `_$0` | anon param | unnamed parameter in constructor/method |
| `!` | exclamation | logical NOT / private method marker |


<!-- index.md -->
# CCO Syntax Tutorial

Welcome! This is a beginner-friendly tutorial for the CCO language.

## What is CCO?

CCO (CNT Configuration Object) is a **human-readable configuration file format**. It is more expressive than JSON, simpler than YAML, and supports type aliases, enumerations, templates (classes), inheritance, and expressions.

> Start with Chapter 1 if you are new to CCO.

## Learning Path

The tutorial is organized into **6 parts, 23 chapters**. Reading in order is recommended.

### Part 1: Meet CCO

| # | Chapter | After this you can... |
|---|---------|---------------------|
| [**1**](01-intro/readme.md) | [What is CCO?](01-intro/readme.md) | Understand what CCO is and how it compares to JSON/YAML |
| [**2**](02-first-file/readme.md) | [Your First .cco File](02-first-file/readme.md) | Write the simplest `.cco` file, understand `key: value` |
| [**3**](03-basic-data/readme.md) | [Basic Data Types](03-basic-data/readme.md) | Use None, booleans, integers (with bases), floats, strings |
| [**4**](04-comments/readme.md) | [Comments](04-comments/readme.md) | Annotate your config with `/* */` |

### Part 2: Organizing Data

| # | Chapter | After this you can... |
|---|---------|---------------------|
| [**5**](05-maps/readme.md) | [Maps -- Key-Value Pairs](05-maps/readme.md) | Group related data with `(k: v, ...)` |
| [**6**](06-arrays/readme.md) | [Arrays -- Lists](06-arrays/readme.md) | Store ordered lists with `(v, v, ...)` |
| [**7**](07-shorthand/readme.md) | [Top-Level Shorthand](07-shorthand/readme.md) | Understand the overall `.cco` file structure |

### Part 3: Making Data Clearer

| # | Chapter | After this you can... |
|---|---------|---------------------|
| [**8**](08-type-basics/readme.md) | [Type Basics](08-type-basics/readme.md) | Understand the five primitive types |
| [**9**](09-typedef/readme.md) | [$typedef -- Type Aliases](09-typedef/readme.md) | Give meaningful names to types |
| [**10**](10-enum/readme.md) | [$enum -- Enumerations](10-enum/readme.md) | Define a fixed set of allowed values |

### Part 4: Templates -- Reusable Structures

| # | Chapter | After this you can... |
|---|---------|---------------------|
| [**11**](11-template/readme.md) | [What is a Template?](11-template/readme.md) | Understand what problem templates solve |
| [**12**](12-template-fields/readme.md) | [Fields and Default Values](12-template-fields/readme.md) | Write template fields with types and defaults |
| [**13**](13-instantiation/readme.md) | [Instantiation -- Positional Args](13-instantiation/readme.md) | Create instances by passing args in order |
| [**14**](14-named-inst/readme.md) | [Named Instantiation](14-named-inst/readme.md) | Pass args by name (clearer, more flexible) |
| [**15**](15-inheritance/readme.md) | [Template Inheritance](15-inheritance/readme.md) | Extend existing templates with new fields |

### Part 5: Advanced Features

| # | Chapter | After this you can... |
|---|---------|---------------------|
| [**16**](16-expressions/readme.md) | [Expressions](16-expressions/readme.md) | Do arithmetic in CCO |
| [**17**](17-comparison-logic/readme.md) | [Comparison & Logic](17-comparison-logic/readme.md) | Use comparison and logical operators |
| [**18**](18-coalesce/readme.md) | [Coalescing Operator \|](18-coalesce/readme.md) | Handle "use default if missing" scenarios |
| [**19**](19-constructors/readme.md) | [Constructors `$function.@`](19-constructors/readme.md) | Custom template initialization logic |
| [**20**](20-static-methods/readme.md) | [Static Methods `$function.#`](20-static-methods/readme.md) | Define and call methods without instances |
| [**21**](21-format/readme.md) | [`$format` Function](21-format/readme.md) | Parse strings as CCO at runtime |

### Part 6: Appendix

| # | Chapter | Content |
|---|---------|---------|
| [**22**](22-grammar/readme.md) | [Grammar Reference](22-grammar/readme.md) | Complete EBNF-style grammar |
| [**23**](23-keywords/readme.md) | [Keywords & Precedence](23-keywords/readme.md) | Full keyword list, operator precedence table |

## Complete Examples

The [learn/source/](source/) directory contains complete `.cco` files corresponding to each chapter. All can be parsed with `cco_parse_full()`.

## Prerequisites

- No software installation needed to read the tutorial
- Running the examples requires the libcco library (see `Readme.md`)
<!-- source/README.md -->
# Source Files

This directory contains complete `.cco` example files referenced by the tutorial chapters.

| File | Chapter | Content |
|------|---------|---------|
| `hello.cco` | Chapter 2 | Simplest `.cco` file |
| `basic.cco` | Chapters 3-7 | All primitive types, maps, arrays, nesting |
| `typedef_enum.cco` | Chapters 8-10 | `$typedef` type aliases, `$enum` enumerations |
| `server.cco` | Chapters 11-15 | Template definition, fields, instantiation, inheritance |
| `constructor.cco` | Chapter 19 | Custom constructors `$function.@` |
| `full_demo.cco` | Comprehensive | Complete config with all features |

All files can be parsed with `cco_parse_full()`.
