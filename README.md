# pplatex

[![CI](https://github.com/dpezto/pplatex/actions/workflows/ci.yml/badge.svg)](https://github.com/dpezto/pplatex/actions/workflows/ci.yml)
[![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](gpl-3.0.txt)

Make LaTeX's log output **readable** — errors and warnings with the file and
line they belong to, and nothing else.

LaTeX can produce beautiful documents, and it produces some of the noisiest
command-line output in computing while doing it. `pplatex` parses the log
stream of latex, pdflatex or lualatex and reduces it to the messages you
actually act on. It turns this

```
]
\openout2 = `chapter.aux'.


No file chapter.tex.
! Undefined control sequence.
l.9 Something \unknown
```

into this

```
warning: No file chapter.tex
 --> ./test.tex
  |
  = likely: LaTeX creates chapter.tex on this run. If it is still missing after a
            second run, the file really is absent.

error: Undefined control sequence
 --> ./test.tex:9:11
  |
9 | Something \unknown
  |           ^^^^^^^^
  |
  = likely: \unknown is not defined and is not a command we know.
            Check the spelling, or define it with \newcommand in the preamble.

1 error, 1 warning, 0 badboxes
```

on a terminal, and the layout it has always printed when redirected — see
[Two layouts](#two-layouts).

This is a maintained fork of
[stefanhepp/pplatex](https://github.com/stefanhepp/pplatex). The
modernization and parser fixes were developed with LLM assistance, verified
against the test suite in `test/`, and have all been offered upstream.

## Features

- **Errors, warnings and badboxes, attributed.** Each message is tagged with
  the source file and line it came from. The file is tracked through the log's
  parenthesis structure — a heuristic, because the log format guarantees
  nothing, hardened against the cases that break it: filenames wrapped across
  lines, directories with spaces, several files opened on one line, and page
  markers glued to a name.
- **The source line, with a caret.** TeX prints where it stopped as a pair of
  lines whose second is padded to the width of the first. That pair is the
  source line and the column, so both are recovered and shown rather than
  concatenated into the message.
- **Advice, where it can be checked.** Undefined commands are resolved against
  a table built by reading TeX Live and verified by compiling: every claim was
  confirmed by a document that loads the package and one that does not, so a
  command the kernel already provides can never be blamed on a package. Where
  nothing is known, nothing is said.
- **One mistake, reported once.** Repeats are counted rather than repeated, and
  errors that only happened because of an earlier one are listed underneath it.
  The bundled `lotsoferrors.log` goes from 374 errors over 1191 lines to four
  messages over 33, because it only ever contained two mistakes.
- **`file:line:error` style supported.** Logs produced with
  `-file-line-error` report errors as `./main.tex:3: ...`; these are parsed
  directly, taking the file and line the log already states.
- **Works with Tectonic.** Tectonic packs several file-opens per line and
  writes bare filenames; the file tracking handles both.
- **Wrapper or filter, your choice.** Run `ppdflatex main.tex` to compile and
  prettify in one step (`-interaction=nonstopmode` is added for you), or keep
  your build system and post-process the log: `pplatex -i main.log`, or pipe
  with `pplatex -i -`.
- **One binary, three names.** The LaTeX engine is chosen from the name the
  program is invoked as: `pplatex` runs latex, `ppdflatex` runs pdflatex,
  `ppluatex` runs lualatex. Any other engine via `--cmd`.
- **Quiet modes and exit codes.** `-b` hides badboxes, `-q` hides warnings
  too; the exit status reflects whether errors were found, so it composes in
  scripts and Makefiles.

`pplatex` does *not* rerun latex for you when references change; that remains
your build system's job (latexmk, a Makefile, …). The parser is derived from
[Kile](https://apps.kde.org/kile/)'s LaTeX output filter.

## Install

### Homebrew (macOS and Linux)

```sh
brew tap dpezto/pplatex
brew install --HEAD dpezto/pplatex/pplatex
```

### Prebuilt binaries

Each [release](https://github.com/dpezto/pplatex/releases) ships portable
tarballs for `linux-x86_64`, `linux-arm64` and `macos-arm64`. The Linux
binaries are fully static, so they run on any distribution without installing
anything — including a Raspberry Pi (64-bit OS). Unpack and put `bin/` on your
`PATH`; the engine-alias symlinks are preserved by the tarball.

### From source

Requires CMake >= 3.20, a C++ compiler, pkg-config and PCRE2:

```sh
sudo apt-get install cmake pkg-config libpcre2-dev   # Debian/Ubuntu/Raspberry Pi OS
brew install cmake pkgconf pcre2                     # macOS
```

```sh
cmake -S . -B build
cmake --build build
sudo cmake --install build
```

`CMAKE_INSTALL_PREFIX` and `DESTDIR` behave as usual; use
`-DCMAKE_INSTALL_PREFIX=$HOME/.local` to install without root. Without
pkg-config, point CMake at PCRE2 with `PCRE2_INCLUDE_DIR`,
`PCRE2_POSIX_LIBRARY` and `PCRE2_LIBRARY`; when linking a PCRE2 DLL on
Windows, add `-DPCRE2_SHARED=ON`.

## Editor integration

[VimTeX](https://github.com/lervag/vimtex) can use pplatex to populate its
quickfix list. Once `pplatex` is on your PATH:

```vim
let g:vimtex_quickfix_method = 'pplatex'
```

LazyVim enables this automatically when it finds `pplatex` on the PATH.

## Usage

Compile and prettify:

```sh
ppdflatex main.tex                       # pdflatex
pplatex main.tex                         # latex
pplatex -c path/to/engine -- main.tex    # any engine, e.g. not on PATH
```

Parse an existing log, or filter a stream:

```sh
pdflatex main.tex          # run your toolchain as usual
pplatex -i main.log        # then prettify the log

pdflatex main.tex | pplatex -i
```

Options:

```
-c, --cmd <cmd>    Execute <cmd> to compile the tex file
-i, --input <file> Parse logfile <file> instead of running latex ('-' for stdin)
-b                 Do not show badbox messages
-q                 Do not show warnings and badbox messages
-v                 Be verbose
-V, --version      Show version info
-h, --help         Show this help

--format=MODE      auto (default), pretty, classic
--color=MODE       auto (default), always, never
--width=N          Wrap the readable output at N columns
--no-collapse      Report repeats and knock-on errors separately
--self-test        Check the built-in hint tables and exit
```

Do not pass an `-interaction` mode that stops to wait for input; pplatex uses
`-interaction=nonstopmode` by default when none is given.

### Two layouts

`--format=auto`, the default, prints the readable layout above when the output
is a terminal, and the original one-message-per-block layout whenever it is
redirected or piped. That distinction exists because pplatex's output is
parsed: VimTeX runs `pplatex -i <log> >tmp` and matches the classic layout
column for column, so it has to stay put when something other than a person is
reading it. Force either with `--format=pretty` or `--format=classic`.

Grouping applies to both, and so do hints: the classic layout carries them as
indented continuation lines, which is the shape it has always used for the rest
of a message, so an editor folds them into the same entry. Hints it is only
guessing at -- the ones labelled `maybe:` -- are left out of that layout, since
a list of things to go and fix is the wrong place for a guess.

`--no-collapse` turns grouping off. It does not turn hints off.

## Development

The parser is a heuristic over an unspecified format, so behavior is pinned by
fixtures: `test/run.sh` feeds every log in `test/` through the binary and
diffs against the expected output committed in `test/expected/`.

```sh
test/run.sh                 # check against expected output
test/run.sh --update        # regenerate after an intended behavior change
```

Regenerated expected files are part of the change: review that diff line by
line, it *is* the behavior change.

Commits follow [Conventional Commits](https://www.conventionalcommits.org);
releases are cut by release-please from those messages, and CI attaches the
portable binaries to each release.

## Credits and license

Written by Stefan Hepp, with a parser taken from the Kile project by Jeroen
Wijnhout, Thorsten Lück and Michel Ludwig; see `COPYRIGHT.txt`. Fork
maintained by Dai López. GPL-3.0 — see `gpl-3.0.txt`.
