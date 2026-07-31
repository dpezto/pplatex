/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef __PPLATEX_RENDER_H__
#define __PPLATEX_RENDER_H__

#include <string>

#include "knowledge.h"
#include "outputinfo.h"
#include "style.h"

/**
 * Everything the readable renderer needs to know about the terminal, resolved
 * once at startup. Passing it in rather than reading the environment keeps
 * rendering a pure function of its inputs, so the same item always produces
 * the same bytes and can be compared against a fixture.
 */
struct RenderOpts
{
	RenderOpts() : width(80), color(false) {}

	int width;
	bool color;
};

/**
 * Render one message the way a compiler would: the complaint, the place it
 * happened, and the line it happened on with the offending text underlined.
 * Ends with a blank line.
 */
std::string renderPretty(const LatexOutputInfo& item, const RenderOpts& opts,
			 const Annotation *hint);

/** The closing tally, e.g. "1 error, 2 warnings, 0 badboxes". */
std::string renderSummary(int errors, int warnings, int badboxes,
			  int shownErrors, int shownWarnings, int shownBadBoxes,
			  const RenderOpts& opts);

/**
 * Make a path fit for reading: relative to the working directory where it can
 * be, under "~" or "<texmf>" where that is shorter, and with whole middle
 * components dropped only if it is still too wide. Never applied to the
 * classic output, which other programs match against, so nothing that parses
 * pplatex ever sees a shortened path.
 */
std::string shortenPath(const std::string& path, const RenderOpts& opts);

#endif
