/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef __PPLATEX_STYLE_H__
#define __PPLATEX_STYLE_H__

#include <cstdio>
#include <string>

/**
 * Presentation settings, resolved once at startup.
 *
 * Format, color and width all key on whether the output stream is a terminal.
 * A terminal gets the readable rendering; anything redirected or piped keeps
 * the exact bytes pplatex has always produced, because things downstream parse
 * them -- vimtex's quickfix integration runs 'pplatex -i <log> ><tmp>' and
 * matches the classic layout column for column.
 */

enum FormatMode  { FORMAT_AUTO, FORMAT_PRETTY, FORMAT_CLASSIC };
enum ColorMode   { COLOR_AUTO, COLOR_ALWAYS, COLOR_NEVER };

/**
 * Escape sequences the renderer writes, or empty strings when color is off.
 * Keeping them behind a table means the renderer has one code path and stays
 * a pure function of its inputs, so its output can be compared byte for byte.
 */
struct Palette
{
	const char *error;
	const char *warning;
	const char *badbox;
	/** The '-->', '|' and '=' rules that frame a message. */
	const char *rule;
	/** Emphasis for the text of a message. */
	const char *bold;
	/** De-emphasis for detail nobody has to read. */
	const char *dim;
	const char *reset;
};

/** The colored table, or an all-empty one when <color> is false. */
const Palette& palette(bool color);

/** Parse an option value. Returns false and leaves <mode> alone if unknown. */
bool parseFormatMode(const std::string& value, FormatMode& mode);
bool parseColorMode(const std::string& value, ColorMode& mode);

/** Resolve FORMAT_AUTO against <stream>. */
bool wantPretty(FormatMode mode, FILE *stream);

/**
 * Resolve COLOR_AUTO against <stream>. In auto mode NO_COLOR suppresses color,
 * CLICOLOR_FORCE demands it, a dumb terminal cannot render it, and otherwise
 * color follows the terminal.
 */
bool wantColor(ColorMode mode, FILE *stream);

/**
 * Width available for output, clamped to a sane range. An explicit <columns>
 * wins; a terminal is then measured through COLUMNS or the kernel. Anything
 * that is not a terminal is always 80, so redirected output stays byte for
 * byte reproducible no matter what the environment says.
 */
int terminalWidth(int columns, FILE *stream);

#endif
