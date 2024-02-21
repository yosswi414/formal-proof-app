# formal-proof-app

This is a development project as part of the coursework in [Topics in Algebra (4Q, 2023)](https://www.kurims.kyoto-u.ac.jp/~tshun/2023q4t.html) to develop a verifier & type inference system, in reference to the textbook shown below.

## Setup
Makefile builds all executables and generates symbolic links to these in the current directory.
```bash
$ make  
```

## Usage

- Reads `input_file` as a definition file and outputs an identical definitions in different notation

```bash
$ ./def_conv.out -f <input_file> [options...]
```

- Reads `input_file` as a definition file and outputs a script file (series of inference rule)
```bash
$ ./genscript.out -f <input_file> [options...]
```

- Reads `input_file` as a script file and outputs a book (Series of judgements)

```bash
$ ./verifier.out -f <input_file> [options...]
```

- If the target definition is in `resource/def_file`, you can generate its script and book in one line
```bash
$ make check-<target name>
# example: when the target definition name is "a3_fig11.29"
$ make check-a3_fig11.29
```

### Options (Common)

- `-f FILE`: Read `FILE` instead of stdin
- `-s`: Suppress output, only verifies the input
- `-h`: Show option help and exit

### Options (`def_conv.out`)
#### Output format
- `-c`: Use the conventional notation (No syntax sugar)
- `-n`: Use the new notation (Redundant brackets are omitted)
- `-r`: Use the rich notation (Intended to be a human-readable format; not readable by above executables)

### Options (`verifier.out`)

#### Input file
- `-l LINES`: Read first `LINES` lines of input and ignore the rest
- `-d def_file`: Read `def_file` for definition reference (The name of definition will be referred to in the output book)
#### Output file
- `-o out_file`: Output to `out_file` instead of stdout
- `-e log_file`: Output the error output to `log_file` instead of stderr
- `--out-def out_def_file`: Extract the final environment from input script and output definitions to `def_file`

#### Output format
- `-c`: Use the conventional notation (No syntax sugar)
- `-n`: Use the new notation (Redundant brackets are omitted)
- `-r`: Use the rich notation (Intended to be a human-readable format; not readable by above executables)
- `-v`: Verbose output (debug purpose)
#### Verification process
- `--skip-check`: Bypass the inference rule applicability check through the script (Saves some time)
- `-i`: Launch in interactive mode (You can edit the script file and see the result immediately)

### Options (`genscript.out`)
- `-o out_file`: Output to `out_file` instead of stdout
- `-t target`: Choose a definition in input and only focus on it and its dependency
- `--dry-run`: Print the dependency list of the target definition
- `-v`: Verbose output (debug purpose)

## Interactive verification (`verifier.out -i`)
You can try to apply the deduction rules of $\lambda \mathrm{D}$ on your own to see how they work. In other words, this interactive mode helps you edit a script file by your hand. If you have your own script generator and it has some flaws that the generated script can't verify a definition, this might be an essential debugging tool to investigate the cause of verification failure.

### Usage
```
$ ./verifier.out -i
[Interactive mode]
Type "help" for available commands.

[#J: 0, #D: 0] $ 
```

See the help text for the detailed features.

### Demo
```
[#J: 0, #D: 0] $ help
[prompt]
#J denotes the number of judgements on the book.
#D denotes the number of defined definitions (by def or defpr).
@n represents that the n-th judgement is refered to as the current context
(if ommited, the last judgement is being referred to)


[general command]
name       args       description
----------------------------------
help                  show this help
exit                  end interactive mode and exit
... (omitted) ...

[derivation command]
name    args         
---------------------
sort                 
var     i var_name   
weak    i j var_name 
form    i j          
... (omitted) ...

[#J: 0, #D: 0] $ load out/test.script
Reading script from out/test.script...
OK: The script has been loaded successfully.

[#J: 10, #D: 0] $ jshow 0 -1
[0]: ∅ ; ∅ ⊢ * : □
[1]: ∅ ; Γ{A:*} ⊢ A : *
[2]: ∅ ; Γ{A:*} ⊢ * : □
[3]: ∅ ; Γ{A:*, B:*} ⊢ B : *
[4]: ∅ ; Γ{A:*, B:*} ⊢ * : □
[5]: ∅ ; Γ{A:*, B:*} ⊢ A : *
[6]: ∅ ; Γ{A:*, B:*, a:A} ⊢ a : A
[7]: ∅ ; Γ{A:*, B:*, a:A} ⊢ * : □
[8]: ∅ ; Γ{A:*, B:*, a:A} ⊢ B : *
[9]: ∅ ; Γ{A:*, B:*} ⊢ Πa:A.B : *

[#J: 10, #D: 0] $ def 0 9 implies
res [10]: Δ{1:implies} ; ∅ ⊢ * : □

[#J: 11, #D: 1] $ weak 10 10 v1
res [11]: Δ{1:implies} ; Γ{v1:*} ⊢ * : □

[#J: 12, #D: 1] $ weak 11 11 v2
res [12]: Δ{1:implies} ; Γ{v1:*, v2:*} ⊢ * : □

[#J: 13, #D: 1] $ var 10 v1
res [13]: Δ{1:implies} ; Γ{v1:*} ⊢ v1 : *

[#J: 14, #D: 1] $ var 11 v2
res [14]: Δ{1:implies} ; Γ{v1:*, v2:*} ⊢ v2 : *

[#J: 15, #D: 1] $ weak 13 11 v2
res [15]: Δ{1:implies} ; Γ{v1:*, v2:*} ⊢ v1 : *

[#J: 16, #D: 1] $ tail
[11]: Δ{1:implies} ; Γ{v1:*} ⊢ * : □
[12]: Δ{1:implies} ; Γ{v1:*, v2:*} ⊢ * : □
[13]: Δ{1:implies} ; Γ{v1:*} ⊢ v1 : *
[14]: Δ{1:implies} ; Γ{v1:*, v2:*} ⊢ v2 : *
[15]: Δ{1:implies} ; Γ{v1:*, v2:*} ⊢ v1 : *

[#J: 16, #D: 1] $ dnlookup implies
implies -> {0}: Def< Γ{A:*, B:*} ▷ implies := Πa:A.B : * >

[#J: 16, #D: 1] $ inst 12 2 14 15 0
res [16]: Δ{1:implies} ; Γ{v1:*, v2:*} ⊢ implies[v2, v1] : *

[#J: 17, #D: 1] $ save out/implies.test
Writing data (233 Bytes) to out/implies.test... OK

[#J: 17, #D: 1] $ init

[#J: 0, #D: 0] $ load out/implies.test
Reading script from out/implies.test...
OK: The script has been loaded successfully.

[#J: 17, #D: 1] $ tail
[12]: Δ{1:implies} ; Γ{v1:*, v2:*} ⊢ * : □
[13]: Δ{1:implies} ; Γ{v1:*} ⊢ v1 : *
[14]: Δ{1:implies} ; Γ{v1:*, v2:*} ⊢ v2 : *
[15]: Δ{1:implies} ; Γ{v1:*, v2:*} ⊢ v1 : *
[16]: Δ{1:implies} ; Γ{v1:*, v2:*} ⊢ implies[v2, v1] : *

[#J: 17, #D: 1] $ jump 14

[#J: 17, #D: 1] @14 $ type v2
Δ{{ implies }} ; Γ{v1:*, v2:*} ⊢ v2 : *

[#J: 17, #D: 1] @14 $ type implies[v1, v2]
Δ{{ implies }} ; Γ{v1:*, v2:*} ⊢ implies[v1, v2] : *

[#J: 17, #D: 1] @14 $ type @ 
TypeError: square is not typable
        at term = □, context = Γ{v1:*, v2:*}

[#J: 17, #D: 1] @14 $ jump -1

[#J: 17, #D: 1] $ exit
$
```

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
