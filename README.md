WANDA is a command-line termination tool.  Given a file describing a
(higher-order) term rewriting system, she will return YES (the system
is terminating), NO (the system is non-terminating) or MAYBE
(termination status could not be determined).

Command-line arguments:
  * <filename>
    If no file name is given, the user is prompted to type in a
    system in AFSM format.
    If several files are given, termination is proved for all of
    them.
  * <time>
    By default, a purely numeric input to WANDA is assumed to be the
    timeout; this number is completely ignored (for the moment).
  * --verbose, -v
    Give additional comments during execution, such as describing
    what the tool is doing.  When verbose is enabled, the proof
    output is printed during executing, rather than afterwards.
  * --silent, -s
    By default WANDA prints YES/NO/MAYBE followed by an explanation
    (a proof, counter example or the reason why deciding was
    difficult).  You can suppress this by choosing silent mode.
  * --rewrite, -r
    Rather than proving termination, the user is prompted to type in
    a term, and this term is rewritten following the rules.
  * --format=<format>, -f <format>
    Rather than choosing the standard formalism associated with a
    file's extension, consider all input files in the given
    formalism.  Choices are currently afsm and atrs.
  * --firstorder=<file>, -i <file>
    WANDA uses a first-order termination prover, located in the
    resources/ directory, to prove termination of the first order
    part of the rules.  With this argument you can indicate which
    file in the resources/ directory should be used.  If none is
    indicated, WANDA will not attempt to use a first-order prover.
    If this is omitted, the file "firstorderprover" will be used.

    WARNING: originally no file "firstorderprover" is present.
    Any first-order termination prover using the input- and
    output-format of the competition can be added here.  If you do
    not have a first order prover available, just use -i none.
  * --firstordernon=<file>, -n <file>
    Similar as --firstorder=<file>, but considering a separate prover
    that is used for non-termination analysis.
  * --disable=<list>, -d <list>
    You can disable certain features of WANDA; for example to
    disable non-termination proving and rules removal, use
    --disable=nt,rr or -d nt,rr (note that there is no space after
    the comma!).  The things you can disable are:
    - nt: non-termination proving
    - rr: rules removal
    - dp: the dependency pair framework
    - static: the static dependency pair framework
    - dynamic: the dynamic dependency pair framework
    - sc: the subterm criterion (in the dependeny pair framework)
    - fr: formative rules
    - ur: usable rules
    - fwrt: formative rules with respect to an argument filtering
    - uwrt: usable rules with respect to an argument filtering
    - local: using tags and suchlike improvements for locality
    - graph: use the dependency graph
    - poly: polynomial interpretations
    - pprod: polynomial interpretations with products
    - horpo: recursive path ordering
  * --query=<property>, -q <property>
    Query whether the system has the given property; outputs YES or
    NO and does nothing else.  The property must be one of the
    following:
    - etalong
    - baseoutputs
    - local
    - pfp
    - strongpfp
    - leftlinear
    - fullyextended
    - algebraic (no meta-variables occur below an abstraction)
    - argumentfree (meta-variables do not take arguments)
  * --show, -w
    Just show the system (possibly converting it to an AFSM first)
    and abort.
  * --debug, -D
    Give additional debug information during execution.  More verbose
    than verbose, this is really just for debugging problems (for
    example, the entire formula table of HORPO is printed).
  * --style=<style>, -y <style>
    Creates output of the given style for the proof; default is plain.
    Possible styles are:
    - html: creates a single html-page
    - plain: just ascii
    - ansi: just ascii, but with ansi-colour characters
    - utf: prints to the terminal using utf8-characters
    - ansiutf: prints to the terminal using utf8-characters and
      ansi-colour
  * --output=<filename>, -o <filename>
    Prints only the YES/NO/MAYBE to stdout, and the rest to the given
    file.

Although multiple input formalisms are supported, they are all
converted to AFSMs, which is WANDA's underlying formalism.

The following extensions are currently supported:
  file.afsm     -- an input file in graphical afsm format
  file.atrs     -- an input file in untyped applicative format
                   (human-readable); this is automatically assigned a
                   monomorphic typing
  file.afs      -- an input file in Jouannaud's AFS format, using
                   simple types (this is a human-readable variation of
                   the competition's xml format)
  file.xml      -- the competition's xml format, encoding an afs

In the future, the following extensions are also intended to be
supported:
  file.prs      -- an input file in Nipkow's PRS format

Example input files of all the supported formalisms are in the
benchmarks/ folder.

Note: a default sat-solver is supplied in the resources/ folder (this
is a version of minisat).  You can replace this by another sat-solver
with similar input and output.

Note: the afsm input format requires all identifiers to be built
exclusively from alphanumberic characters (and potentially the special
character '!').

---------------------------------------------------------------------
CONVERTER is a tool for converting between the four formalisms named
above (although conversion to .atrs is not possible).
Just run converter.exe without further arguments for an overview of
the syntax.

