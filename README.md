General Info
============

This is a maintained fork of [stefanhepp/pplatex](https://github.com/stefanhepp/pplatex).
The modernization and parser fixes were developed with LLM assistance, verified
against the test suite in `test/`, and have all been offered upstream.

LaTeX is able to produce really nice document layouts. But it is also able to
produce a lot of noise on the command line.  `pplatex` is a command-line tool
that parses the logs of latex and pdflatex and prints warnings and errors in an
human readable format.

`pplatex` will transform something like this
```
]
\openout2 = `chapter.aux'.


No file chapter.tex.
! Undefined control sequence.
l.9 Something \unknown
```
into this
```
** Warning in ./test.tex: No file chapter.tex.

** Error   in ./test.tex, Line 9:
   Undefined control sequence Something \unknown

Result: o) Errors: 1, Warnings: 1, BadBoxes: 0
```

The code is based on the LaTeX output parser of [Kile](http://kile.sourceforge.net/) (also used by TexMakerX),
with some modifications and bugfixes. Be aware that since the log output of the
LaTeX tools is not well defined (in any sense of the word), parsing is done by
a heuristic that tries its best but still might fail in some cases (e.g., having
very long directory names with spaces or special characters *might* cause issues).

In contrast to [rubber](https://launchpad.net/rubber), pplatex does *not* run your latex tools multiple times 
when references change or compile your images or the like. This remains the task
of you / your makefile.


Install
=======

Homebrew (macOS and Linux)
--------------------------

    brew install dpezto/pplatex/pplatex

This installs `pplatex` along with the `ppdflatex` and `ppluatex` aliases.

From source
-----------

You need CMake >= 3.20, a C++ compiler, pkg-config and PCRE2. On Debian and
Ubuntu:

    sudo apt-get install cmake pkg-config libpcre2-dev

On macOS with Homebrew:

    brew install cmake pkgconf pcre2

Then build and install:

    cmake -S . -B build
    cmake --build build
    sudo cmake --install build

`CMAKE_INSTALL_PREFIX` and `DESTDIR` work as usual, so to install somewhere
else without root:

    cmake -S . -B build -DCMAKE_INSTALL_PREFIX=$HOME/.local
    cmake --build build
    cmake --install build

The install step places `pplatex` in `bin/` and adds `ppdflatex` and `ppluatex`
next to it as symlinks. All three are the same program: it picks the LaTeX
engine from the name it was invoked as.

Without pkg-config, point CMake at PCRE2 by hand with `PCRE2_INCLUDE_DIR`,
`PCRE2_POSIX_LIBRARY` and `PCRE2_LIBRARY`. When linking against a PCRE2 DLL on
Windows rather than a static library, also pass `-DPCRE2_SHARED=ON`.


Editor integration
==================

[VimTeX](https://github.com/lervag/vimtex) can use pplatex to populate its
quickfix list. Once `pplatex` is on your PATH:

    let g:vimtex_quickfix_method = 'pplatex'

LazyVim enables this automatically when it finds `pplatex` on the PATH.


Quick Start
===========

In your latex project directory, just run

    ppdflatex myfile.tex

Use pplatex instead to run latex instead of pdflatex.

If your latex tools are not in your PATH, use

    pplatex -c path/to/pdflatex -- myfile.tex

You can also use pplatex to parse an existing log file ...

    # run pdflatex normally
    pdflatex myfile.tex
    # Process the logfile and print warnings and errors.
    pplatex -i myfile.log

... or to filter the output of pdflatex/xelatex/lualatex:

    pdflatex myfile.tex | pplatex -i


Usage
=====

If the pdflatex and latex tools are in your PATH (try running 'latex' on your 
commandline), you can simply use ppdflatex if you want to run pdflatex, and pplatex 
for latex, like

    pplatex myfile.tex

or 

    ppdflatex myfile.tex

Warnings and badbox messages can be hidden like this

    pplatex -q -- latexfile.tex

In order to parse an existing log file, use

    pplatex -i somefile.log

To specify which latex program should be used (p.e. when latex is not in PATH), use the 
--cmd option, like

    pplatex -c /path/to/latex.exe -- <latex options> myfile

Make sure you do not use an interaction mode where latex waits for user input on 
errors. pplatex uses -interaction=nonstopmode by default if no interaction mode is 
specified.

All three binaries are the same program; pplatex picks the engine from the name
it was invoked as. `pplatex` runs latex, `ppdflatex` runs pdflatex and
`ppluatex` runs lualatex. Any other name is rejected, so use `--cmd` to run an
engine that has no dedicated name.


Open Tasks
==========

- [ ] Support warnings and error messages of PGF / TikZ
- [ ] Check for bugfixes in updates in Kile's parser and integrate them. Submit bugfixes in pplatex to Kile.

