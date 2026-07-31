/* Which package owns an internal namespace.
 *
 * Package authors prefix their private macros, so \Gm@lmargin belongs to
 * geometry and \Hy@something to hyperref. An author never types these, which
 * makes them the most useful thing to recognise: when one is undefined the
 * document is not at fault, and advice about the document cannot help. The
 * right thing to say is which package is misconfigured.
 *
 * Hand written. Deriving this by scanning the distribution produces prefixes
 * that are wrong in ways that matter -- "caption" maps to caption3, "beamer"
 * to beamerbaseoverlay, and a single letter "f" to fancyhdr, which would claim
 * every macro beginning \f@ for it. The mapping has to name the package an
 * author would actually load, so it is written out rather than generated.
 *
 * Longest prefix wins, so a specific namespace beats a shorter one that
 * happens to share its opening letters.
 */

static const struct {
    const char *prefix;
    const char *package;
} NAMESPACE_OWNERS[] = {
    { "BKM",    "bookmark" },
    { "Gin",    "graphics" },
    { "Gm",     "geometry" },
    { "Geom",   "beamer" },
    { "Hy",     "hyperref" },
    { "KV",     "keyval" },
    { "LT",     "longtable" },
    { "MT",     "microtype" },
    { "NAT",    "natbib" },
    { "SI",     "siunitx" },
    { "Url",    "url" },
    { "XC",     "xcolor" },
    { "adjbox", "adjustbox" },
    { "alg",    "algorithmic" },
    { "bbl",    "babel" },
    { "beamer", "beamer" },
    { "blx",    "biblatex" },
    { "bm",     "bm" },
    { "calc",   "calc" },
    { "cref",   "cleveref" },
    { "csq",    "csquotes" },
    { "enit",   "enumitem" },
    { "etb",    "etoolbox" },
    { "FV",     "fvextra" },
    { "lst",    "listings" },
    { "mdf",    "mdframed" },
    { "minted", "minted" },
    { "pgf",    "pgf" },
    { "scr",    "koma-script" },
    { "tcb",    "tcolorbox" },
    { "tikz",   "pgf" },
    { "ttl",    "titlesec" },
};
