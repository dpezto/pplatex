/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "render.h"

#include <cstdlib>
#include <sstream>
#include <vector>

#if defined( __WIN32__ ) || defined( _WIN32 )

#include <direct.h>
#define getcwd(buf,size) _getcwd(buf,size)

#else

#include <unistd.h>

#endif

using namespace std;

static string spaces(size_t n)
{
    return string(n, ' ');
}

static string trimRight(const string& s)
{
    size_t end = s.find_last_not_of(" \t");
    return (end == string::npos) ? "" : s.substr(0, end+1);
}

/**
 * Drop one closing full stop.
 *
 * Whether a message keeps its own is an accident of which pattern matched it:
 * a plain TeX error is recognised by the period and loses it, while the same
 * error in file:line:error style keeps it. Headlines read as labels here, so
 * take it off either way and let them agree.
 */
static string dropFullStop(const string& s)
{
    if ( s.length() > 1 && s[s.length()-1] == '.' ) {
	return s.substr(0, s.length()-1);
    }
    return s;
}

static string toString(int n)
{
    ostringstream s;
    s << n;
    return s.str();
}

/** True if <s> has <c> at <pos>. */
static bool isControlChar(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '@';
}

/**
 * Offset and length of the last control sequence in <text>, or length 0 if it
 * does not end in one.
 *
 * TeX says so itself, in the help it prints under an undefined control
 * sequence: "The control sequence at the end of the top line of your error
 * message was never \def'ed." So the token worth pointing at is the last one.
 */
static void lastControlSequence(const string& text, size_t& start, size_t& length)
{
    start = 0;
    length = 0;

    for (size_t i = 0; i < text.length(); i++) {
	if ( text[i] != '\\' ) {
	    continue;
	}

	size_t end = i + 1;
	while ( end < text.length() && isControlChar(text[end]) ) {
	    end++;
	}

	if ( end > i + 1 ) {
	    start = i;
	    length = end - i;
	}
    }
}

/**
 * The span to underline in the source line: the command TeX choked on where
 * there is one, otherwise a single mark at the point it stopped.
 */
static void underlineSpan(const TexContext& context, size_t& start, size_t& length)
{
    size_t csStart, csLength;

    // TeX stops just past the offending command, so it sits at the end of the
    // text already consumed. Point at the command, not at the character after.
    lastControlSequence(context.before, csStart, csLength);
    if ( csLength > 0 && csStart + csLength == context.before.length() ) {
	start = csStart;
	length = csLength;
	return;
    }

    // Nothing consumed yet: whatever comes next is what it could not read.
    if ( context.before.empty() ) {
	lastControlSequence(context.after, csStart, csLength);
	if ( csLength > 0 && csStart == 0 ) {
	    start = 0;
	    length = csLength;
	    return;
	}
    }

    start = context.before.length();
    length = 1;
}

string shortenPath(const string& path, const RenderOpts& opts)
{
    if ( opts.paths == PATH_FULL || path.empty() ) {
	return path;
    }

    string shortened = path;

    // Below the working directory, which is where the interesting files are.
    char buffer[4096];
    if ( getcwd(buffer, sizeof(buffer)) ) {
	string cwd(buffer);
	if ( !cwd.empty() && shortened.compare(0, cwd.length(), cwd) == 0
	     && shortened.length() > cwd.length() && shortened[cwd.length()] == '/' ) {
	    shortened = shortened.substr(cwd.length()+1);
	}
    }

    const char *home = getenv("HOME");
    if ( home && home[0] != 0 ) {
	string prefix(home);
	if ( shortened.compare(0, prefix.length(), prefix) == 0
	     && shortened.length() > prefix.length() && shortened[prefix.length()] == '/' ) {
	    shortened = "~" + shortened.substr(prefix.length());
	}
    }

    // Anything under a texmf tree belongs to the distribution, not to the
    // author. The name says "not yours to edit" in far fewer columns, and the
    // angle brackets cannot begin a real path, so it is never ambiguous.
    size_t texmf = shortened.find("/texmf");
    if ( texmf != string::npos ) {
	size_t rest = shortened.find('/', texmf+1);
	if ( rest != string::npos ) {
	    shortened = "<texmf>" + shortened.substr(rest);
	}
    }

    // Still too wide: drop whole directories out of the middle. Never a partial
    // name -- half of a directory is not something anyone can act on -- and
    // never the file itself.
    size_t budget = (opts.width > 12) ? (size_t)opts.width - 12 : 12;
    if ( shortened.length() > budget ) {
	vector<string> parts;
	size_t start = 0;
	for (size_t i = 0; i <= shortened.length(); i++) {
	    if ( i == shortened.length() || shortened[i] == '/' ) {
		parts.push_back(shortened.substr(start, i-start));
		start = i+1;
	    }
	}

	// An absolute path splits with an empty first component; the root it
	// stands for still has to be printed.
	size_t first = (parts.size() > 1 && parts[0].empty()) ? 1 : 0;
	string head = (first == 1) ? "/" + parts[1] : parts[0];

	if ( parts.size() > first + 2 ) {
	    // Keep as much of the tail as the budget allows: the directories
	    // nearest the file are the ones that identify it.
	    string tail = parts[parts.size()-1];
	    for (size_t i = parts.size()-1; i > first + 1; i--) {
		string longer = parts[i-1] + "/" + tail;
		if ( head.length() + 5 + longer.length() > budget ) {
		    break;
		}
		tail = longer;
	    }

	    shortened = head + "/.../" + tail;
	}
    }

    return shortened;
}

/** "error", "warning" or "badbox", with the package that raised it. */
static string severityLabel(const LatexOutputInfo& item, const Palette& pal, string& color)
{
    switch (item.type()) {
	case LatexOutputInfo::itmError:   color = pal.error;   return "error";
	case LatexOutputInfo::itmWarning: color = pal.warning; return "warning";
	case LatexOutputInfo::itmBadBox:  color = pal.badbox;  return "badbox";
	default:                          color = pal.bold;    return "note";
    }
}

string renderPretty(const LatexOutputInfo& item, const RenderOpts& opts)
{
    const Palette& pal = palette(opts.color);

    ostringstream out;

    string severityColor;
    string severity = severityLabel(item, pal, severityColor);

    const char *vertical = opts.unicode ? "\xe2\x94\x82" : "|";
    const char *arrow    = opts.unicode ? "\xe2\x94\x80\xe2\x96\xb6" : "-->";

    // "warning[hyperref]:" reads the way a compiler's error codes do, and puts
    // the package that is actually complaining in front of its message.
    string qualifier;
    if ( !item.package().empty() ) {
	qualifier = "[" + item.package() + "]";
    }

    string headline = dropFullStop(trimRight(item.headline()));

    out << severityColor << severity << qualifier << ":" << pal.reset
	<< " " << pal.bold << headline << pal.reset << "\n";

    // The gutter is as wide as the line number it has to hold.
    bool hasLine = item.sourceLine() > 0;
    string lineno = hasLine ? toString(item.sourceLine()) : "";
    size_t gutter = hasLine ? lineno.length() : 1;

    // Work out what to underline first, because the column reported alongside
    // the file has to be the start of it. TeX stops just past the command that
    // upset it, and sending an editor there would put the cursor one token
    // late, pointing at text that is not the problem.
    bool hasSource = false;
    string source;
    size_t span = 0, spanLength = 0;

    if ( item.hasSourceContext() && hasLine ) {
	const TexContext& context = item.sourceContext();
	source = trimRight(context.before + context.after);

	if ( !source.empty() ) {
	    underlineSpan(context, span, spanLength);
	    hasSource = true;
	}
    }

    string location = shortenPath(item.source(), opts);
    if ( !location.empty() ) {
	out << spaces(gutter) << pal.rule << arrow << pal.reset << " " << location;

	if ( hasLine ) {
	    out << ":" << lineno;

	    // Only when it points somewhere real; see sourceColumn().
	    if ( hasSource && item.sourceColumn() > 0 ) {
		out << ":" << (span + 1);
	    }
	}
	out << "\n";
    }

    string bar = spaces(gutter+1) + pal.rule + vertical + pal.reset;

    // The source line, rebuilt out of the two halves TeX printed, with the
    // command it stopped at underlined.
    if ( hasSource ) {
	out << bar << "\n";
	out << pal.rule << lineno << pal.reset << " " << pal.rule << vertical
	    << pal.reset << " " << source << "\n";
	out << bar << " " << spaces(span) << severityColor
	    << string(spanLength, '^') << pal.reset << "\n";
    }

    // What TeX was expanding when it stopped. Shown verbatim, because that is
    // how it appears in the log everyone already knows how to read. Detail
    // lines hang off an '=' with no bar, the way a compiler note does.
    string note = spaces(gutter+1) + pal.rule + "=" + pal.reset + " ";
    bool notes = false;

    for (size_t i = 0; i < item.trace().size(); i++) {
	const TexContext& frame = item.trace()[i];
	string text = trimRight(frame.tag + frame.before);

	if ( text.empty() ) {
	    continue;
	}

	if ( !notes ) {
	    out << bar << "\n";
	    notes = true;
	}

	string label = (i == 0) ? "expanding: " : "           ";

	out << note << pal.dim << label << pal.reset << text << "\n";

	size_t start, length;
	lastControlSequence(text, start, length);
	if ( length > 0 ) {
	    out << spaces(gutter+3) << spaces(label.length() + start) << severityColor
		<< string(length, '^') << pal.reset << "\n";
	}
    }

    out << "\n";

    return out.str();
}

/** "1 error" but "2 errors". */
static string count(int n, const char *singular, const char *plural)
{
    return toString(n) + " " + (n == 1 ? singular : plural);
}

string renderSummary(int errors, int warnings, int badboxes, const RenderOpts& opts)
{
    const Palette& pal = palette(opts.color);

    ostringstream out;

    out << (errors   ? pal.error   : pal.dim) << count(errors, "error", "errors")     << pal.reset << ", "
	<< (warnings ? pal.warning : pal.dim) << count(warnings, "warning", "warnings") << pal.reset << ", "
	<< (badboxes ? pal.badbox  : pal.dim) << count(badboxes, "badbox", "badboxes")  << pal.reset << "\n";

    return out.str();
}
