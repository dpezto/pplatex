/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "knowledge.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "regex.h"

using namespace std;

#include "hints_table.inc"
#include "prefixes.inc"
#include "symbols.inc"

static const size_t HINT_COUNT = sizeof(HINTS) / sizeof(HINTS[0]);
static const size_t OWNER_COUNT = sizeof(NAMESPACE_OWNERS) / sizeof(NAMESPACE_OWNERS[0]);
static const size_t SYMBOL_COUNT = sizeof(SYMBOLS) / sizeof(SYMBOLS[0]);
static const size_t KERNEL_COUNT = sizeof(KERNEL_COMMANDS) / sizeof(KERNEL_COMMANDS[0]);

/** Longest a hint line may be, so two of them fit under a message. */
static const size_t MAX_HINT_WIDTH = 76;

const char *Annotation::label() const
{
    switch (confidence) {
	case CONF_CERTAIN: return "hint";
	case CONF_GUESS:   return "maybe";
	default:           return "likely";
    }
}

static string trimRight(const string& s)
{
    size_t end = s.find_last_not_of(" \t.");
    return (end == string::npos) ? "" : s.substr(0, end+1);
}

/** Which package declares <command>, or "" if the table does not know. */
static string symbolPackage(const string& command)
{
    size_t low = 0, high = SYMBOL_COUNT;

    while ( low < high ) {
	size_t mid = low + (high - low) / 2;
	int order = command.compare(SYMBOLS[mid].command);

	if ( order == 0 ) {
	    return SYMBOL_PACKAGES[SYMBOLS[mid].package];
	}
	if ( order < 0 ) {
	    high = mid;
	} else {
	    low = mid + 1;
	}
    }

    return "";
}

/**
 * Which package owns the internal namespace <command> sits in.
 *
 * Longest prefix wins, so a specific namespace beats a shorter one that
 * happens to share its opening letters.
 */
static string namespaceOwner(const string& command)
{
    size_t at = command.find('@');
    if ( at == string::npos || at <= 1 ) {
	return "";
    }

    string prefix = command.substr(1, at - 1);
    string owner;
    size_t best = 0;

    for (size_t i = 0; i < OWNER_COUNT; i++) {
	string candidate(NAMESPACE_OWNERS[i].prefix);

	if ( candidate.length() > best && candidate.length() <= prefix.length()
	     && prefix.compare(0, candidate.length(), candidate) == 0 ) {
	    owner = NAMESPACE_OWNERS[i].package;
	    best = candidate.length();
	}
    }

    return owner;
}

/** Damerau-Levenshtein distance, giving up once it passes <limit>. */
static size_t editDistance(const string& a, const string& b, size_t limit)
{
    size_t n = a.length(), m = b.length();

    if ( n > m + limit || m > n + limit ) {
	return limit + 1;
    }

    vector<size_t> prev2(m+1, 0), prev(m+1), curr(m+1);

    for (size_t j = 0; j <= m; j++) {
	prev[j] = j;
    }

    for (size_t i = 1; i <= n; i++) {
	curr[0] = i;
	size_t best = curr[0];

	for (size_t j = 1; j <= m; j++) {
	    size_t cost = (a[i-1] == b[j-1]) ? 0 : 1;

	    size_t value = prev[j] + 1;
	    if ( curr[j-1] + 1 < value )   value = curr[j-1] + 1;
	    if ( prev[j-1] + cost < value ) value = prev[j-1] + cost;

	    // Transposition, which is what a mistyped pair of letters is.
	    if ( i > 1 && j > 1 && a[i-1] == b[j-2] && a[i-2] == b[j-1]
		 && prev2[j-2] + 1 < value ) {
		value = prev2[j-2] + 1;
	    }

	    curr[j] = value;
	    if ( value < best ) {
		best = value;
	    }
	}

	if ( best > limit ) {
	    return limit + 1;
	}

	prev2 = prev;
	prev = curr;
    }

    return prev[m];
}

/**
 * The one command in the table closest to <command>, or "" when there is no
 * single answer.
 *
 * Silence is the right output whenever more than one candidate is equally
 * close. A spelling suggestion is only worth making if it is the obvious one;
 * offering a guess picked out of a tie is worse than offering nothing, because
 * the reader has no way to tell which kind they were given.
 */
static string nearestCommand(const string& command, string& package)
{
    // Too short to correct: at three characters everything is two edits from
    // everything else.
    if ( command.length() < 5 ) {
	return "";
    }

    size_t limit = command.length() < 7 ? 1 : 2;
    size_t best = limit + 1, ties = 0;
    string found;

    for (size_t i = 0; i < SYMBOL_COUNT; i++) {
	string candidate(SYMBOLS[i].command);
	size_t distance = editDistance(command, candidate, limit);

	if ( distance > limit ) {
	    continue;
	}
	if ( distance < best ) {
	    best = distance;
	    ties = 1;
	    found = candidate;
	    package = SYMBOL_PACKAGES[SYMBOLS[i].package];
	}
	else if ( distance == best ) {
	    ties++;
	}
    }

    // The kernel names as well, or the commonest typo of all goes unanswered:
    // \superseteq is a misspelling of \supseteq, which no package provides
    // because the kernel already does.
    for (size_t i = 0; i < KERNEL_COUNT; i++) {
	string candidate(KERNEL_COMMANDS[i]);
	size_t distance = editDistance(command, candidate, limit);

	if ( distance > limit ) {
	    continue;
	}
	if ( distance < best ) {
	    best = distance;
	    ties = 1;
	    found = candidate;
	    package = "";
	}
	else if ( distance == best ) {
	    ties++;
	}
    }

    if ( found.empty() || ties > 1 ) {
	return "";
    }

    return found;
}

/**
 * The command TeX could not read.
 *
 * TeX states the rule itself, in the help it prints under the error: "The
 * control sequence at the end of the top line of your error message was never
 * \def'ed." So it is the last command on the first line of the context,
 * whether that line is the source echo, a reader marker, or a macro that was
 * being expanded when TeX gave up.
 */
static string undefinedCommand(const LatexOutputInfo& item, bool& typedHere)
{
    string text;
    typedHere = false;

    if ( !item.trace().empty() ) {
	const TexContext& frame = item.trace()[0];
	text = frame.tag + frame.before;
    }
    else if ( item.hasSourceContext() ) {
	text = item.sourceContext().before;
	typedHere = true;
    }

    string found;
    for (size_t i = 0; i < text.length(); i++) {
	if ( text[i] != '\\' ) {
	    continue;
	}

	size_t end = i + 1;
	while ( end < text.length()
		&& (isalpha((unsigned char)text[end]) || text[end] == '@') ) {
	    end++;
	}

	if ( end > i + 1 ) {
	    found = text.substr(i, end - i);
	}
    }

    return found;
}

/**
 * Work out what to say about an undefined command.
 *
 * The rungs run from most specific to least, and each one exists because the
 * rung below it would have given an answer that is wrong.
 */
static bool explainUndefined(const LatexOutputInfo& item, const DocContext& doc,
			     Annotation& out)
{
    bool typedHere = false;
    string command = undefinedCommand(item, typedHere);

    if ( command.empty() ) {
	return false;
    }

    ostringstream first, second;

    // (a) A package internal. The author did not write this, so no advice
    // about their document can be right, and suggesting they install
    // something would be actively misleading.
    if ( command.find('@') != string::npos ) {
	string owner = namespaceOwner(command);

	out.confidence = CONF_LIKELY;
	if ( !owner.empty() ) {
	    first << command << " is " << owner << "'s internal macro, not yours.";
	    second << (doc.loaded(owner)
		       ? "That package is misconfigured, or the version installed is too old."
		       : "Something is expanding its macros without loading it.");
	} else {
	    first << command << " is a package internal, marked by the @ in its name.";
	    second << "You did not write it: this is a package conflict, not your mistake.";
	}
	out.first = first.str();
	out.second = second.str();
	return true;
    }

    string package = symbolPackage(command);

    // (b) Known, and the package is already there. Naming it would send the
    // reader to add a line that is present.
    if ( !package.empty() && doc.loaded(package) ) {
	out.confidence = CONF_LIKELY;
	first << command << " comes from " << package << ", which is already loaded.";
	second << "Something redefined it, or it was used before the package was.";
	out.first = first.str();
	out.second = second.str();
	return true;
    }

    // (c) Known and absent. The one case where naming a package is the answer.
    if ( !package.empty() ) {
	out.confidence = typedHere ? CONF_CERTAIN : CONF_GUESS;
	first << command << " comes from " << package
	      << ". Add \\usepackage{" << package << "}.";
	out.first = first.str();
	out.second = second.str();
	return true;
    }

    // (d) Unknown, but one command in the table is close enough to name.
    string nearPackage;
    string near = nearestCommand(command, nearPackage);
    if ( !near.empty() ) {
	out.confidence = CONF_GUESS;
	first << "Nothing defines " << command << ". Did you mean " << near << "?";
	if ( nearPackage.empty() ) {
	    second << near << " is part of LaTeX itself; no package is needed.";
	} else if ( !doc.loaded(nearPackage) ) {
	    second << near << " needs \\usepackage{" << nearPackage << "}.";
	}
	out.first = first.str();
	out.second = second.str();
	return true;
    }

    // (e) Nothing recognised it.
    out.confidence = CONF_LIKELY;
    first << command << " is not defined and is not a command we know.";
    second << "Check the spelling, or define it with \\newcommand in the preamble.";
    out.first = first.str();
    out.second = second.str();
    return true;
}

/** Does <text> match entry <i> of the hint table? */
static bool matchesHint(size_t i, const string& text, const LatexOutputInfo& item,
			Regex **compiled)
{
    if ( HINTS[i].severity != SEV_ANY && HINTS[i].severity != item.type() ) {
	return false;
    }

    if ( HINTS[i].package[0] != 0 && item.package() != HINTS[i].package ) {
	return false;
    }

    switch (HINTS[i].kind) {
	case 'P':
	    return text.compare(0, strlen(HINTS[i].pattern), HINTS[i].pattern) == 0;
	case 'S':
	    return text.find(HINTS[i].pattern) != string::npos;
	default:
	    if ( !compiled[i] ) {
		compiled[i] = new Regex(HINTS[i].pattern);
	    }
	    return compiled[i]->match(text);
    }
}

/** Replace \1, \2 ... in <hint> with what the pattern captured. */
static string interpolate(const string& hint, Regex *rx, const string& text)
{
    if ( !rx || hint.find('\\') == string::npos ) {
	return hint;
    }

    string out;
    for (size_t i = 0; i < hint.length(); i++) {
	if ( hint[i] == '\\' && i + 1 < hint.length()
	     && hint[i+1] >= '1' && hint[i+1] <= '9' ) {
	    out += rx->getMatch(text, hint[i+1] - '0');
	    i++;
	} else {
	    out += hint[i];
	}
    }

    return out;
}

bool annotate(const LatexOutputInfo& item, const DocContext& doc, Annotation& out)
{
    // Compiled lazily and kept: a log can carry hundreds of messages and there
    // is no reason to rebuild the same pattern for each of them.
    static Regex **compiled = new Regex*[HINT_COUNT]();

    string text = trimRight(item.headline());

    if ( text.empty() ) {
	return false;
    }

    // The computed explanation first: it knows which command is undefined and
    // what the document already loaded, so it beats anything the table can say.
    if ( item.type() == LatexOutputInfo::itmError
	 && text.compare(0, 27, "Undefined control sequence") == 0 ) {
	if ( explainUndefined(item, doc, out) ) {
	    return true;
	}
    }

    for (size_t i = 0; i < HINT_COUNT; i++) {
	if ( !matchesHint(i, text, item, compiled) ) {
	    continue;
	}

	out.confidence = HINTS[i].confidence;
	out.first  = interpolate(HINTS[i].first,  compiled[i], text);
	out.second = interpolate(HINTS[i].second, compiled[i], text);
	return true;
    }

    return false;
}

/** How far past a message a consequence of it may still appear, in log lines. */
static const int CASCADE_WINDOW = 30;

/**
 * Which messages follow from which. Left is the mistake, right is what TeX
 * says next because of it.
 */
static const struct {
    const char *root;
    const char *follower;
} CASCADES[] = {
    /* A command that does not exist expands to nothing, so whatever was meant
     * to come out of it is missing. */
    { "Undefined control sequence", "Missing number, treated as zero" },
    { "Undefined control sequence", "Illegal unit of measure" },
    { "Undefined control sequence", "Missing $ inserted" },
    { "Undefined control sequence", "Missing \\endcsname inserted" },
    { "Undefined control sequence", "Missing { inserted" },
    { "Undefined control sequence", "Missing } inserted" },
    { "Undefined control sequence", "Argument of" },

    /* A missing number is replaced by zero, and the unit is still absent. */
    { "Missing number, treated as zero", "Illegal unit of measure" },

    /* Once TeX is confused about math mode it stays confused. */
    { "Missing $ inserted", "Missing $ inserted" },
    { "Missing $ inserted", "Display math should end with $$" },

    /* TeX emits these as a pair. */
    { "Runaway argument", "Paragraph ended before" },
    { "Runaway argument", "File ended while scanning use of" },

    /* The row structure is already broken. */
    { "Extra alignment tab", "Misplaced \\noalign" },
};

static const size_t CASCADE_COUNT = sizeof(CASCADES) / sizeof(CASCADES[0]);

static bool startsWith(const string& text, const char *prefix)
{
    return text.compare(0, strlen(prefix), prefix) == 0;
}

bool isConsequenceOf(const LatexOutputInfo& root, const LatexOutputInfo& follower)
{
    if ( root.type() != LatexOutputInfo::itmError
	 || follower.type() != LatexOutputInfo::itmError ) {
	return false;
    }

    // Same place, or it is a different problem that happens to look similar.
    if ( root.source() != follower.source()
	 || root.sourceLine() != follower.sourceLine() ) {
	return false;
    }

    if ( follower.outputLine() - root.outputLine() > CASCADE_WINDOW ) {
	return false;
    }

    string a = trimRight(root.headline());
    string b = trimRight(follower.headline());

    for (size_t i = 0; i < CASCADE_COUNT; i++) {
	if ( startsWith(a, CASCADES[i].root) && startsWith(b, CASCADES[i].follower) ) {
	    return true;
	}
    }

    // Whatever went wrong, this is TeX reporting that it gave up over it.
    if ( startsWith(b, "Emergency stop")
	 || startsWith(b, "Fatal error occurred") ) {
	return true;
    }

    return false;
}

string groupKey(const LatexOutputInfo& item)
{
    ostringstream key;

    key << item.type() << '\x1f' << item.package() << '\x1f' << item.source() << '\x1f';

    string text = trimRight(item.headline());

    // A badbox is the same finding wherever it happens and however far over it
    // runs, so the amount is dropped from the key. Numbers stay in everything
    // else: "Unicode character (U+00E9)" and "(U+00F1)" are different problems.
    if ( item.type() == LatexOutputInfo::itmBadBox ) {
	size_t open = text.find(" (");
	if ( open != string::npos ) {
	    size_t close = text.find(')', open);
	    if ( close != string::npos ) {
		text = text.substr(0, open) + text.substr(close+1);
	    }
	}
    }

    key << text;

    return key.str();
}

int selfTestKnowledge()
{
    static Regex **compiled = new Regex*[HINT_COUNT]();

    int problems = 0;

    for (size_t i = 0; i < HINT_COUNT; i++) {
	string id(HINTS[i].id);

	// Ids are what a report refers to, so they have to be unique.
	for (size_t j = 0; j < i; j++) {
	    if ( id == HINTS[j].id ) {
		cerr << "duplicate id: " << id << endl;
		problems++;
	    }
	}

	// Measured on the template. A hint that interpolates a capture can
	// still come out wider once a long file name is substituted in, which
	// the terminal wraps; there is no width to check against here.
	if ( strlen(HINTS[i].first) > MAX_HINT_WIDTH
	     || strlen(HINTS[i].second) > MAX_HINT_WIDTH ) {
	    cerr << id << ": hint line over " << MAX_HINT_WIDTH << " columns" << endl;
	    problems++;
	}

	if ( HINTS[i].first[0] == 0 ) {
	    cerr << id << ": no hint text" << endl;
	    problems++;
	}

	// Wording the table is not allowed to use, because it fills space
	// without telling the reader what to do.
	static const char *banned[] = { "consider", "you might", "check your packages",
					"there may be", "perhaps you should", 0 };
	string both = string(HINTS[i].first) + " " + HINTS[i].second;
	for (size_t b = 0; banned[b]; b++) {
	    if ( both.find(banned[b]) != string::npos ) {
		cerr << id << ": hedging phrase \"" << banned[b] << "\"" << endl;
		problems++;
	    }
	}

	// The example has to match its own entry, and no earlier one. A general
	// pattern placed above a specific one silently swallows it, which is how
	// this table goes wrong as it grows.
	LatexOutputInfo probe;
	probe.setType(HINTS[i].severity == SEV_ANY
		      ? LatexOutputInfo::itmError : HINTS[i].severity);
	if ( HINTS[i].package[0] != 0 ) {
	    probe.setPackage("Package", HINTS[i].package);
	}
	probe.setMessage(HINTS[i].example);

	string example = trimRight(HINTS[i].example);
	size_t hit = HINT_COUNT;

	for (size_t j = 0; j < HINT_COUNT; j++) {
	    if ( matchesHint(j, example, probe, compiled) ) {
		hit = j;
		break;
	    }
	}

	if ( hit == HINT_COUNT ) {
	    cerr << id << ": its own example matches nothing" << endl;
	    problems++;
	} else if ( hit != i ) {
	    cerr << id << ": its example is claimed by " << HINTS[hit].id
		 << ", which comes first" << endl;
	    problems++;
	}
    }

    // Every package the table names has to be one the symbol data knows, or
    // the advice points at something that may not exist.
    for (size_t i = 0; i < OWNER_COUNT; i++) {
	if ( strlen(NAMESPACE_OWNERS[i].prefix) < 2 ) {
	    cerr << "namespace prefix too short to be safe: "
		 << NAMESPACE_OWNERS[i].prefix << endl;
	    problems++;
	}
    }

    // The symbol table is binary searched, so it has to be sorted.
    for (size_t i = 1; i < SYMBOL_COUNT; i++) {
	if ( strcmp(SYMBOLS[i-1].command, SYMBOLS[i].command) >= 0 ) {
	    cerr << "symbols.inc is not sorted at " << SYMBOLS[i].command << endl;
	    problems++;
	    break;
	}
    }

    cout << HINT_COUNT << " hints, " << SYMBOL_COUNT << " commands, "
	 << OWNER_COUNT << " namespaces: "
	 << (problems ? "FAILED" : "ok") << endl;

    return problems;
}
