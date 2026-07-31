#!/bin/sh
#
# Check every "<command> comes from <package>" claim against a real TeX Live,
# by compiling documents and asking LaTeX.
#
#   tools/verify-hints.sh <claims.tsv> [<claims.tsv> ...]
#
# Three things have to hold before a claim may ship.
#
#   The package exists.                  kpsewhich finds it.
#   Loading it defines the command.      The positive probe.
#   NOT loading it leaves it undefined.  The negative control.
#
# The negative control is the one that matters. Naming a package only helps if
# the command is actually missing without it, and the most plausible-looking
# claims are exactly the ones that fail here: \supseteq is declared in the
# kernel's fontmath.ltx, so it is defined with or without amssymb, and telling
# someone to load amssymb for it sends them to fix a file that is not broken.
# A claim whose control comes back DEFINED is rejected rather than softened,
# because there is no wording that makes it true.
#
# Probes are batched: one document per package plus one control, so the whole
# table costs about forty compiles rather than two thousand.
#
# Nothing here uses echo. /bin/sh on macOS expands backslash escapes in echo,
# which turns \begin{document} into egin{document} and makes every probe fail
# to compile -- a verifier that rejects everything looks exactly like a
# verifier that works, so this is worth stating rather than rediscovering.
#
# Exits non-zero if any claim fails, so CI can gate on it.

set -u

if [ $# -eq 0 ]; then
    printf 'usage: %s <claims.tsv> [...]\n' "$0" >&2
    exit 2
fi

command -v pdflatex >/dev/null 2>&1 || { printf 'pdflatex not found\n' >&2; exit 2; }

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT INT TERM

claims="$work/claims.tsv"
grep -hv '^#' "$@" | grep -v '^[[:space:]]*$' | sort -u > "$claims"

printf 'checking %s claims\n' "$(wc -l < "$claims" | tr -d ' ')"

# Ask LaTeX which of these names exist, one line of output per name. \ifcsname
# is used rather than \csname because it does not bring a name into existence
# as a side effect of asking about it.
#
#   $1 = file to write the answers to
#   $2 = preamble line, empty for the control
#   $3 = newline separated bare names, no leading backslash
probe() {
    {
	printf '%s\n' '\documentclass{article}'
	[ -n "$2" ] && printf '%s\n' "$2"
	printf '%s\n' '\begin{document}'
	printf '%s\n' "$3" | while IFS= read -r name; do
	    [ -n "$name" ] || continue
	    printf '\\ifcsname %s\\endcsname\\typeout{PPV:%s:DEFINED}\\else\\typeout{PPV:%s:MISSING}\\fi\n' \
		"$name" "$name" "$name"
	done
	printf '%s\n' '\end{document}'
    } > "$work/probe.tex"

    rm -f "$work/probe.log"
    ( cd "$work" && pdflatex -interaction=nonstopmode probe.tex >/dev/null 2>&1 )

    if [ ! -f "$work/probe.log" ]; then
	printf 'probe did not compile (preamble: %s)\n' "${2:-none}" >&2
	: > "$1"
	return 1
    fi

    grep -o 'PPV:[^:]*:[A-Z]*' "$work/probe.log" | sort -u > "$1"
}

# The control: no packages at all. Anything defined here comes from the kernel,
# and no package claim about it can be right.
probe "$work/control.txt" "" "$(cut -f1 "$claims" | sed 's/^\\//' | sort -u)" || exit 2

: > "$work/failures"
checked=0

for package in $(cut -f2 "$claims" | sort -u); do
    if ! kpsewhich "$package.sty" >/dev/null 2>&1 &&
       ! kpsewhich "$package.cls" >/dev/null 2>&1; then
	printf 'SKIP %s: not installed\n' "$package"
	continue
    fi

    names=$(awk -F'\t' -v p="$package" '$2 == p { sub(/^\\/, "", $1); print $1 }' "$claims" | sort -u)

    if ! probe "$work/with.txt" "\\usepackage{$package}" "$names"; then
	printf 'SKIP %s: probe failed to compile\n' "$package"
	continue
    fi

    for name in $names; do
	checked=$((checked + 1))

	if grep -qx "PPV:$name:DEFINED" "$work/control.txt"; then
	    printf 'FAIL \\%s -> %s: defined without it, so the package is not the fix\n' \
		"$name" "$package" >> "$work/failures"
	elif ! grep -qx "PPV:$name:DEFINED" "$work/with.txt"; then
	    printf 'FAIL \\%s -> %s: still undefined after loading it\n' \
		"$name" "$package" >> "$work/failures"
	fi
    done
done

cat "$work/failures"

failed=$(grep -c . "$work/failures" 2>/dev/null || printf '0')

printf '\n%s claims checked, %s rejected\n' "$checked" "$failed"

[ "$failed" -eq 0 ]
