#!/bin/sh
#
# Emit "<command>\t<package>" for the commands a package defines, by reading
# the packages themselves. The file that defines a command is the authority on
# which package provides it, so nothing here is inferred.
#
#   tools/mine-cs.sh [<texmf-dist>] > cs-index.tsv
#
# Two restrictions keep the output trustworthy rather than merely large.
#
# Only the packages listed below are read. Scanning the whole distribution
# yields 14k rows for 8.6k commands, because packages that re-implement other
# packages -- lwarp mirrors nearly everything for its HTML output -- claim
# commands they do not own. \toprule then resolves to five packages, none of
# them booktabs. A command claimed by the wrong package is worse than one that
# is missing: it produces confident advice that cannot work.
#
# Only forms that declare a command an author would type are read. Adding \def
# and \let brings in roughly a thousand internal names -- \HyPsd@..., and the
# like -- that nobody writes. The cost is that a handful of commands defined
# with \def are missed, \includegraphics, \toprule and \mathbb among them;
# those live in tools/symbols.tsv, which is hand written and wins conflicts.
#
# Commands the LaTeX kernel already defines are excluded outright. Naming a
# package for one of those is the most common way for advice about an undefined
# command to be wrong: \supseteq looks exactly like something amssymb would
# provide and is in fact declared in base/fontmath.ltx, so no package will fix
# it and suggesting one sends the reader to fix the wrong thing.

set -u

texmf=${1:-$(kpsewhich -var-value TEXMFDIST 2>/dev/null)}

if [ -z "$texmf" ] || [ ! -d "$texmf" ]; then
    echo "usage: $0 [<texmf-dist>]   (or install TeX Live so kpsewhich works)" >&2
    exit 2
fi

# Packages an author is likely to use a command from and forget to load. Adding
# one here is cheap; the check in tools/verify-hints.sh will catch it if the
# claim does not hold.
packages='
algorithm2e algorithmicx amsfonts amsmath amssymb amsthm
bm booktabs braket cancel caption cleveref
enumitem fancyhdr geometry graphics graphicx
hyperref listings longtable mathtools mdframed microtype multirow
natbib pgfplots physics siunitx stmaryrd subcaption
tabularx textcomp tikz titlesec tcolorbox upgreek url wasysym xcolor
'

forms='\\(DeclareMathSymbol|DeclareMathDelimiter|DeclareMathAccent|DeclareMathOperator|newcommand|providecommand|DeclareRobustCommand|NewDocumentCommand|DeclareTextSymbol)'

# The command name is the last backslash-word of the match, whether the form
# brace-wraps its argument or not.
extract() {
    grep -hoE "$forms\\*?[{ ]*\\\\[A-Za-z@]+" "$@" 2>/dev/null |
	sed 's/.*\\/\\/'
}

kernel=$(mktemp)
trap 'rm -f "$kernel"' EXIT INT TERM

extract "$texmf"/tex/latex/base/*.ltx "$texmf"/tex/latex/base/*.sty |
    sort -u > "$kernel"

echo "# tools/mine-cs.sh: $texmf" >&2
echo "# kernel commands excluded: $(wc -l < "$kernel" | tr -d ' ')" >&2

for package in $packages; do
    file=$(kpsewhich "$package.sty" 2>/dev/null)
    if [ -z "$file" ]; then
	echo "# not installed, skipped: $package" >&2
	continue
    fi

    # A @ in the name marks a package internal. The author never typed it, so
    # no advice about their document can turn on it.
    extract "$file" | grep -v '@' | sort -u |
	while IFS= read -r cs; do
	    printf '%s\t%s\n' "$cs" "$package"
	done
done |
awk -F'\t' 'NR==FNR { kernel[$0]=1; next } !($1 in kernel)' "$kernel" - |
sort -u
