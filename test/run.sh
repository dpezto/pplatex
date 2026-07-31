#!/bin/bash
#
# Run every fixture in test/ through pplatex and compare against the expected
# output committed alongside it.
#
#   test/run.sh [<path-to-pplatex>]     check against test/expected/
#   test/run.sh --update [<pplatex>]    rewrite test/expected/ from the binary
#
# Every fixture is checked twice: once as it renders when redirected, which is
# the classic layout other programs parse, and once with the readable layout
# forced on. The two are compared against test/expected/ and
# test/expected/pretty/ respectively.
#
# Regenerating is expected when a change is meant to alter output. Read the
# resulting diff line by line before committing it: every changed line is either
# the improvement you intended or a regression.

set -u

testdir=$(cd "$(dirname "$0")" && pwd)
expected="$testdir/expected"
prettyexpected="$expected/pretty"

update=false
if [ "${1:-}" = "--update" ]; then
    update=true
    shift
fi

# Default to the build directory used by the README, then fall back to PATH.
pplatex=${1:-}
if [ -z "$pplatex" ]; then
    if [ -x "$testdir/../build/src/pplatex" ]; then
	pplatex="$testdir/../build/src/pplatex"
    else
	pplatex=$(command -v pplatex)
    fi
fi

if [ ! -x "$pplatex" ]; then
    echo "no pplatex binary found; build one or pass a path" >&2
    exit 2
fi

# Resolve before the cd below, or a path given relative to the caller's
# directory -- which is how CI passes it -- stops resolving.
case $pplatex in
    /*) ;;
    *) pplatex=$(cd "$(dirname "$pplatex")" && pwd)/$(basename "$pplatex") ;;
esac

mkdir -p "$expected" "$prettyexpected"

# The readable layout shortens paths against the working directory and $HOME,
# so both have to be pinned or the output depends on who ran the tests.
cd "$testdir" || exit 2
HOME=/nonexistent
export HOME

# Fixed width and charset so the layout does not follow the terminal.
prettyopts="--format=pretty --color=never --charset=ascii --width=100 --paths=short"

failed=0
checked=0

for log in "$testdir"/*.log; do
    name=$(basename "$log" .log)
    want="$expected/$name.txt"

    # pplatex exits non-zero when the log contained errors, which is not a
    # failure of the run itself.
    got=$("$pplatex" -i "$log" 2>&1)
    # shellcheck disable=SC2086
    gotpretty=$("$pplatex" $prettyopts -i "$log" 2>&1)

    if $update; then
	printf '%s\n' "$got" > "$want"
	printf '%s\n' "$gotpretty" > "$prettyexpected/$name.txt"
	echo "updated $name"
	continue
    fi

    for axis in classic pretty; do
	if [ "$axis" = classic ]; then
	    have=$got
	    file=$want
	else
	    have=$gotpretty
	    file="$prettyexpected/$name.txt"
	fi

	checked=$((checked + 1))

	if [ ! -f "$file" ]; then
	    echo "FAIL $name ($axis): no expected output, run '$0 --update'"
	    failed=$((failed + 1))
	elif ! printf '%s\n' "$have" | diff -u "$file" - > "/tmp/pplatex-$name-$axis.diff"; then
	    echo "FAIL $name ($axis):"
	    sed 's/^/    /' "/tmp/pplatex-$name-$axis.diff"
	    failed=$((failed + 1))
	else
	    echo "ok   $name ($axis)"
	fi
    done
done

$update && exit 0

echo
if [ $failed -eq 0 ]; then
    echo "$checked checks, all matching"
else
    echo "$checked checks, $failed failing"
fi

exit $((failed > 0))
