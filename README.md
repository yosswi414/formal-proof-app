# formal-proof-app

This is a development project as part of the coursework in [Topics in Algebra (4Q, 2023)](https://www.kurims.kyoto-u.ac.jp/~tshun/2023q4t.html) to develop a verifier & type inference system, in reference to the textbook shown below.

## Setup

```bash
$ make all  # builds all executables
```

## Usage

- Reads `input_file` as a definition file and outputs an identical definition file in different notation

```bash
$ bin/def_conv.out -f <input_file> [options...]
```

- Reads `input_file` as a script file and outputs a book (Series of judgements)

```bash
$ bin/verifier.out -f <input_file> [options...]
```

### Options (Common)

- `-f FILE`: Read `FILE` instead of stdin
- `-c`: Use the conventional notation (No syntax sugar)
- `-n`: Use the new notation (Redundant brackets are omitted)
- `-r`: Use the rich notation (Intended to be a human-readable format; not readable by above executables)
- `-s`: Suppress output, only verifies the input
- `-h`: Show help page and exit

### Options (Only available in `bin/verifier.out`)

- `-l LINES`: Read first `LINES` lines of input and ignore the rest
- `-d def_file`: Read `def_file` for definition reference (The name of definition will be referred to in the output book)
- `-o out_file`: Output the output (book) to `out_file` instead of stdout
- `-e log_file`: Output the error output to `log_file` instead of stderr
- `--out-def out_def_file`: Extract the final environment from input script and output definitions to `def_file`
- `--skip-check`: Bypass the inference rule applicability check through the script (Saves some time)

## About the syntax sugars

### Lambda expressions ( $\varepsilon$ )

- Backward-compatible (Accepts any conventional notation)
- Variable can have a longer name, just as constant does
  - e.g. `%%Leq m_1 m_2` ($Leq\ m_1\ m_2$)
    - Old style: `%(%(L)(M))(m)` (Variables could only have their name of length 1)
  - Name collision (using the name of already defined constant as a variable) is prohibited
- Binary operators (`->` (kind), `=>` (implies[]), `<=>` (equiv[]))
  - e.g. `?P: S->* . (%P x => %P y)` $\left(\Pi P:S\rightarrow *\ .\ P\ x\Rightarrow P\ y\right)$
    - Old style: `?P:(?x:(S).(*)).(implies[(%(P)(x)),(%(P)(y))])`

### Definition file ( $\Delta$ )

Including the conventional way, we have three ways to describe definitions.

Examples below shows different way to describe the exact same definitions (`implies`, `implies_in`, `implies_el`, `contra`, `forall`, `refl`).

Single-line comment (`//`) and multi-line comment (`/*` - `*/`) are also available.

These three different notation can coexist in a single file.

#### Method 1 (Conventional) (Output format option: `-c`)

```plain
def2
2
A
*
B
*
implies
?a:(A).(B)
*
edef2

def2
3
A
*
B
*
u
?y:(A).(B)
implies_in
u
implies[(A),(B)]
edef2

def2
4
A
*
B
*
u
?x:(A).(B)
v
A
implies_el
%(u)(v)
B
edef2

def2
0
contra
?A:(*).(A)
*
edef2

def2
2
S
*
P
?a:(S).(*)
forall
?a:(S).(%(P)(a))
*
edef2

def2
2
S
*
l
?a:(S).(?b:(S).(*))
refl
forall[(S),($a:(S).(%(%(l)(a))(a)))]
*
edef2

END
```

#### Method 2 (Newer) (Output format option: `-n`)

```plain
def2
2
A : *
B : *
implies := ?a.A.B : *
edef2

def2
3
A : *
B : *
u : ?y:A.B
implies_in := u : implies[A, B]
edef2

def2
4
A : *
B : *
u : ?x:A.B
v : A
implies_el := %u v : B
edef2

def2
0
contra := ?a:*.a : *
edef2

def2
2
S : *
P : ?a:S.*
forall := ?a:S.%P a : *
edef2

def2
2
S : *
l : ?a:S.?b:S.*
refl := forall[S, $a:S.%%l a a] : *
edef2

END
```

#### Method 3 (Flag style-like notation)  (Output format option: N/A)
```plain
[A: *, B: *]
| implies := A->B : *
| [u: A->B]
| | implies_in := u : implies[A, B] // alternative notation: A => B
| | [v: A]
| | | implies_el := %u v : B

contra := ?A:*.A : *

[S: *]
| [P: S->*]
| | forall := ?a:S.%P a : *
| [Leq: S->S->*]
| | refl := forall[S, $a:S.%%Leq a a] : *

END
```

## Reference

- Textbook: [Type Theory and Formal Proof: An Introduction](https://www.cambridge.org/core/books/type-theory-and-formal-proof/0472640AAD34E045C7F140B46A57A67C) (PDF: Free as of 2023/12/21)
  - Mostly referring to Chapter 11.
