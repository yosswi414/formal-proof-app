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

## Reference

- Textbook: [Type Theory and Formal Proof: An Introduction](https://www.cambridge.org/core/books/type-theory-and-formal-proof/0472640AAD34E045C7F140B46A57A67C) (PDF: Free as of 2023/12/21)
  - Mostly referring to Chapter 11.
