/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "style.h"

#include <cstdlib>
#include <string>

#if defined( __WIN32__ ) || defined( _WIN32 )

#include <io.h>
#define isatty(x) _isatty(x)
#define fileno(x) _fileno(x)

#else

#include <unistd.h>
#include <sys/ioctl.h>

#endif

using namespace std;

/** Smallest and largest width the pretty renderer is willing to lay out for. */
static const int MIN_WIDTH = 40;
static const int MAX_WIDTH = 120;

/** Width used whenever the output is not a terminal, so tests are stable. */
static const int DEFAULT_WIDTH = 80;

static const Palette COLORED = {
    "\033[1;31m",  /* error   */
    "\033[1;33m",  /* warning */
    "\033[1;34m",  /* badbox  */
    "\033[1;34m",  /* rule    */
    "\033[1m",     /* bold    */
    "\033[2m",     /* dim     */
    "\033[0m"      /* reset   */
};

static const Palette PLAIN = { "", "", "", "", "", "", "" };

const Palette& palette(bool color)
{
    return color ? COLORED : PLAIN;
}

bool parseFormatMode(const string& value, FormatMode& mode)
{
    if ( value == "auto" )    { mode = FORMAT_AUTO;    return true; }
    if ( value == "pretty" )  { mode = FORMAT_PRETTY;  return true; }
    if ( value == "classic" ) { mode = FORMAT_CLASSIC; return true; }
    return false;
}

bool parseColorMode(const string& value, ColorMode& mode)
{
    if ( value == "auto" )   { mode = COLOR_AUTO;   return true; }
    if ( value == "always" ) { mode = COLOR_ALWAYS; return true; }
    if ( value == "never" )  { mode = COLOR_NEVER;  return true; }
    return false;
}

static bool isTerminal(FILE *stream)
{
    return stream && isatty(fileno(stream));
}

bool wantPretty(FormatMode mode, FILE *stream)
{
    switch (mode) {
	case FORMAT_PRETTY:  return true;
	case FORMAT_CLASSIC: return false;
	default:             return isTerminal(stream);
    }
}

/** An environment variable that is set and not empty. */
static bool envSet(const char *name)
{
    const char *value = getenv(name);
    return value && value[0] != 0;
}

bool wantColor(ColorMode mode, FILE *stream)
{
    if ( mode == COLOR_ALWAYS ) {
	return true;
    }
    if ( mode == COLOR_NEVER ) {
	return false;
    }

    // https://no-color.org: set and non-empty, whatever the value.
    if ( envSet("NO_COLOR") ) {
	return false;
    }

    // The inverse convention, as used by the BSD tools: force color even
    // when the output is redirected. "0" means the opposite of set.
    const char *force = getenv("CLICOLOR_FORCE");
    if ( force && force[0] != 0 && string(force) != "0" ) {
	return true;
    }

    const char *term = getenv("TERM");
    if ( term && string(term) == "dumb" ) {
	return false;
    }

    return isTerminal(stream);
}

static int clampWidth(int width)
{
    if ( width < MIN_WIDTH ) {
	return MIN_WIDTH;
    }
    if ( width > MAX_WIDTH ) {
	return MAX_WIDTH;
    }
    return width;
}

int terminalWidth(int columns, FILE *stream)
{
    if ( columns > 0 ) {
	return clampWidth(columns);
    }

    // Not a terminal: never measure anything. A COLUMNS inherited from an
    // interactive shell would otherwise make redirected output depend on the
    // window that happened to launch it, and the fixture tests compare bytes.
    if ( !isTerminal(stream) ) {
	return DEFAULT_WIDTH;
    }

    const char *env = getenv("COLUMNS");
    if ( env ) {
	int width = atoi(env);
	if ( width > 0 ) {
	    return clampWidth(width);
	}
    }

#ifdef TIOCGWINSZ
    struct winsize ws;
    if ( ioctl(fileno(stream), TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 ) {
	return clampWidth(ws.ws_col);
    }
#endif

    return DEFAULT_WIDTH;
}
