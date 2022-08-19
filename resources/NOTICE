SOME NOTES ABOUT THE FILES IN THIS DIRECTORY:

=== firstorderprover

"firstorderprover" is NaTT, a first-order termination analysis tool,
as configured for StarExec in the 2019 termination competition.

Note that NaTT relies on the smt-solver Z3.  Z3 is available under the
MIT licence.  The text of this licence can be found in the licences/
directory.

New versions of NaTT are available from
  https://www.trs.css.i.nagoya-u.ac.jp/NaTT/


To plug in another first-order termination tool, it suffices if this
tool accepts the human-readable input format and command-line
arguments from the competition.  Note, however, that the tool will be
invoked from the main WANDA directory, while it resides inside
resources/ itself.  This may affect calls to external files.

=== firstordernonprover

NOT INCLUDED.

You can add "firstordernonprover": a potentially different termination tool
that is optimised to prove non-termination.  In particular, this version
should supply a counterexample for termination if possible.


=== satsolver

"satsolver" is MiniSat, a solver for satisfiability problems.  The
source is available in minisat-2.2.0.tar.gz
To compile from source, use:
  > tar xvzf minisat-2.2.0.tar.gz
  > cd minisat/
  > export root=<current directory>
      (if this doesn't work, try export MROOT=<current directory>)
  > cd simp
  > make
And then move the resulting file "minisat" to "satsolver" in the
resources directory.

MiniSat is released under the MIT licence.  The text of this licence
can be found in the licences/ directory.


To plug in another sat-solver, it suffices if this solver accepts the
format from the sat-competition.  However, note, as before, that the
tool will be invoked from the main WANDA directory.


=== timeout

"timeout" is a simple bash script to mimic the GNU core utility of
the same name.  It does not seem to be licenced.  The script
originates from http://www.odi.ch/prog/timeout.php .
It does not need to be compiled, just set it to be executable if this
is not already the case, using
  > chmod oug+x timeout

