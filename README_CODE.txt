TERM REWRITING CORE
  type             -- defines polymorphic types
  typesubsitution  -- maps type variables to types, can be applied on both
                      types and terms
  term             -- defines (meta-)terms
  alphabet         -- maps names of constants to their (minimal) type
  environment      -- keeps track of variables with their types and
                      possibly their name
  varset           -- ADT - just a list of variable indexes
  substitution     -- maps variables (and meta-variables) to terms, can be
                      applied on terms
  rule             -- generic class for all kinds of rewrite rules
  beta             -- the Beta rule
  matchrule        -- rules in AIDTS format (closed polymorphic rules with
                      meta-variables)

INPUT MODULE
  typer            -- used for type unifying and determining minimal type
  textconverter    -- converts textual description of AIDTS-terms to
                      program constructs
  inputreaderaidts -- used for reading an aidts either from a file or from
                      user input
  inputreaderatrs  -- used to read and assign some typing to an applicative
                      trs
  inputreaderafs   -- used to read a file or string describing an AFS
                      (which may have function applications)
  xmlreader        -- transforms the competition's xml format into a string
                      describing an AFS (in the format from inputreaderafs)
  afs              -- encodes an AFS, and transforms it to an AIDTS

DEPENDENCY PAIR FRAMEWORK
  dependencypair   -- ADT - encodes a dependency pair
  dependencygraph  -- the dependency graph, has functionality to calculate
                      itself and to find strongly connected components
  rulesmanipulator -- used for finding properties of rules (such as
                      left-linearity and orthogonality) and to calculate
                      formative and usable rules
  dpframework      -- the dependency pair framework: starts the first order
                      prover and the dependency graph, and uses HORPO to
                      solve requirements for SCCs until no dependency pairs
                      remain in the graph
  requirement      -- ADT - requirement to send to constraint solving
  reqmodifier      -- used for manipulating the ordering requirements for
                      the dependency pair framework, before being sent to
                      HORPO

CONSTRAINT SOLVING
  firstorder       -- checks which part of the rules can be considered
                      'first order' and feeds those to an external tool
MAIN PROGRAM
  nonterminator    -- does some simple tests for obviously non-terminating
                      rules
  wanda            -- parses runtime arguments and sets everything in
                      motion

OTHER
  converter        -- does conversions between the formalisms; this file is
                      NOT linked in with WANDA, but automatically compiled
                      along with the same makefile

