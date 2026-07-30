class Pplatex < Formula
  desc "Prettify latex/pdflatex output into readable errors and warnings"
  homepage "https://github.com/stefanhepp/pplatex"
  url "https://github.com/dpezto/pplatex/archive/refs/tags/v1.1.0.tar.gz"
  sha256 "4436bed444d3e0d9d9b568a666c1ab2737b12c81867ee300a6fc025d20acb33f"
  license "GPL-3.0-or-later"
  head "https://github.com/dpezto/pplatex.git", branch: "master"

  depends_on "cmake" => :build
  depends_on "pkgconf" => :build
  depends_on "pcre2"

  def install
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    (testpath/"t.log").write <<~LOG
      This is pdfTeX, Version 3.14159265 (preloaded format=latex)
      **t.tex
      (./t.tex

      LaTeX Warning: Reference `sec:intro' on page 1 undefined on input line 15.

      ! Undefined control sequence.
      l.9 Something \\unknown
      )
    LOG

    # pplatex exits 1 when the log contained errors
    output = shell_output("#{bin}/pplatex -i #{testpath}/t.log", 1)

    # The message must survive intact past its embedded colon
    assert_match "Reference `sec:intro' on page 1 undefined", output
    assert_match "Undefined control sequence", output
    assert_match "Errors: 1, Warnings: 1", output

    # Each name selects its own engine from argv[0]. The engine is announced
    # before it is spawned, so this holds whether or not LaTeX is installed;
    # the exit status depends on that and is deliberately discarded.
    %w[pplatex latex ppdflatex pdflatex ppluatex lualatex].each_slice(2) do |name, engine|
      assert_match "Executing: #{engine}",
                   shell_output("#{bin}/#{name} -v -- x.tex 2>&1 || true")
    end
  end
end
