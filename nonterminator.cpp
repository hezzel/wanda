/**************************************************************************
   Copyright 2012, 2013 Cynthia Kop

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 *************************************************************************/

#include "nonterminator.h"
#include "typesubstitution.h"
#include "substitution.h"
#include "environment.h"
#include "outputmodule.h"
#include <iostream>

NonTerminator :: NonTerminator(Alphabet &Sigma,
                               vector<MatchRule*> _rules,
                               bool _immediate)
    :rules(_rules), immediate_beta(_immediate) {

  // copy constants
  vector<string> constants = Sigma.get_all();
  for (int i = 0; i < constants.size(); i++)
    F.add(constants[i], Sigma.query_type(constants[i])->copy());

  // initialise beta
  beta = new Beta;
}

NonTerminator :: ~NonTerminator() {
  delete beta;
}

/* ========== check for an obvious loop ========== */

bool NonTerminator :: obvious_loop(MatchRule *rule) {
  // take the left-hand side of the rule, and make a term out of it
  PTerm base = copy_as_term(rule->query_left_side());

  // perform a breadth-first search on reductions
  vector<PTerm> reduction;
  vector<int> previous;
  vector<int> depth;
  Rule *beta = (immediate_beta ? new Beta() : NULL);
  int counter = -1;
  string subpos;
  reduction.push_back(base);
  previous.push_back(-1);
  depth.push_back(0);
  for (int i = 0; i < reduction.size() && counter == -1; i++) {
    vector<Rule*> apprule;
    vector<string> apppos;
    possible_reductions(reduction[i], apprule, apppos);
    for (int j = 0; j < apprule.size(); j++) {
      // we only need to bother checking whether this result
      // instantiates base if it instantiates the rule which base
      // was derived from
      if (i > 0 && apprule[j] == dynamic_cast<Rule*>(rule)) {
        TypeSubstitution theta;
        Substitution gamma;
        if (base->instantiate(reduction[i]->subterm(apppos[j]),
            theta, gamma) &&
            reachable(reduction[i], apppos[j], gamma)) {
          counter = i;
          subpos = apppos[j];
          break;
        }
      }
      if (reduction.size() < 100 && depth[i] < 5) {
        PTerm t = reduction[i]->copy();
        t = apprule[j]->apply(t, apppos[j]);
        if (immediate_beta) t = beta->normalise(t);
        reduction.push_back(t);
        previous.push_back(i);
        depth.push_back(depth[i]+1);
      }
    }
  }

  bool foundloop = counter != -1;
  if (counter != -1) {
    vector<PTerm> red;
    while (counter != -1) {
      red.push_back(reduction[counter]);
      counter = previous[counter];
    }
    red.pop_back();

    wout.print("It is easy to see that this system is non-terminating:\n");
    ArList arities = wout.arities_for_system(F, rules);
    map<int,string> metanaming, freenaming, boundnaming;
    wout.start_reduction(wout.print_term(base, arities, F, metanaming,
                                         freenaming, boundnaming));
    for (int j = red.size()-1; j >= 0; j--) {
      boundnaming.clear();
      wout.continue_reduction(
          wout.reduce_arrow(),
          wout.print_term(red[j], arities, F, metanaming, freenaming,
                          boundnaming));
    }

    if (subpos != "") {
      wout.continue_reduction(
          wout.superterm_symbol(),
          wout.print_term(red[0]->subterm(subpos), arities, F,
                          metanaming, freenaming, boundnaming));
    }
    wout.end_table();
    if (subpos != "") {
      wout.print("That is, a term s reduces to a term t which has a "
        "subterm that is an instance of the original term.");
    }
    else {
      wout.print("That is, a term s reduces to a term t which "
        "instantiates s.");
    }
  }

  for (int j = 0; j < reduction.size(); j++) delete reduction[j];

  return foundloop;
}

PTerm NonTerminator :: copy_as_term(PTerm term) {
  if (term->query_constant() || term->query_variable()) return term->copy();
  if (term->query_abstraction()) {
    PTerm sub = copy_as_term(term->get_child(0));
    PVariable var = dynamic_cast<PVariable>(term->get_child(-1)->copy());
    return new Abstraction(var, sub);
  }
  if (term->query_application()) {
    PTerm a = copy_as_term(term->get_child(0));
    PTerm b = copy_as_term(term->get_child(1));
    return new Application(a,b);
  }
  if (term->query_meta()) {
    MetaApplication *meta = dynamic_cast<MetaApplication*>(term);
    PTerm ret = meta->get_metavar()->copy();
    for (int i = 0; i < meta->number_children(); i++)
      ret = new Application(ret, copy_as_term(meta->get_child(i)));
    return ret;
  }
  // huh - don't know what this is
  return term->copy();
}

bool NonTerminator :: reachable(PTerm term, string subpos, Substitution &gamma) {
  if (!immediate_beta) return true;

  if (subpos == "") return true;
  if (term->query_abstraction())
    return reachable(term->get_child(0), subpos.substr(1), gamma);
  if (term->query_application() && subpos[0] == '1')
    return reachable(term->get_child(0), subpos.substr(1), gamma);
  if (term->query_application() && subpos[0] == '2') {
    PTerm head = term->query_head();
    if (head->query_variable()) {
      PVariable var = dynamic_cast<PVariable>(head);
      if (gamma.contains(var) && !gamma[var]->equals(var)) return false;
    }
    return reachable(term->get_child(1), subpos.substr(1), gamma);
  }
  return false;
}

/* ========== check whether a rule implements lambda calculus ========== */

bool NonTerminator :: lambda_calculus(MatchRule *rule) {
  PTerm left = rule->query_left_side();
  PTerm right = rule->query_right_side();

  // special case: a reduction l -> X_{alpha}
  if (right->query_meta() && right->query_type()->query_typevar() &&
      right->subterm("0") == NULL) {
    PVariable X = dynamic_cast<MetaApplication*>(right)->get_metavar();
    vector<string> Xpos = find_metavar(left, X);
    if (Xpos.size() != 1) return false;
      // this trick can't deal with non-left-linearity
    string basepos = Xpos[0];
    while (true) {
      basepos = find_base_superterm(left, basepos);
      if (basepos == "") return false;
      PTerm sub = left->subterm(basepos);
      if (sub->query_type()->equals(right->query_type())) continue;
      Varset Xvars = X->query_type()->vars();
      Varset subvars = sub->query_type()->vars();
      bool overlap = false;
      for (Varset::iterator it = Xvars.begin(); it != Xvars.end(); it++)
        if (subvars.contains(*it)) overlap = true;
      if (overlap) continue;
      TypeSubstitution subst;
      PType o = sub->query_type();
      subst[dynamic_cast<TypeVariable*>(right->query_type())] =
        new ComposedType(o->copy(),o->copy());
      left = left->copy();
      right = right->copy();
      left->apply_type_substitution(subst);
      right->apply_type_substitution(subst);
      PVariable Z = new Variable(o->copy());
      vector<PTerm> dummy;
      PTerm mv = new MetaApplication(Z, dummy);
      left = new Application(left, mv);
      right = new Application(right, mv->copy());
      vector<string> lposY; lposY.push_back("2");
      omega(left, right, "1"+basepos, "1"+Xpos[0], "", lposY, 0);
      delete left;
      delete right;
      return true;
    }
  }

  // we don't want rules of functional type, so append meta-variables
  // if necessary
  left = left->copy();
  right = right->copy();
  while (left->query_type()->query_composed()) {
    PVariable Z = new Variable(left->query_type()->query_child(0)->copy());
    vector<PTerm> args;
    MetaApplication *MZ = new MetaApplication(Z, args);
    left = new Application(left, MZ);
    right = new Application(right, MZ->copy());
  }

  // other cases: first "fix" meta-applications /\x1...xn.Z[x1...xn]
  vector<string> varlist = clean_metavariables(left, right);
  if (varlist.size() == 0) {
    delete left; delete right;
    return false;
  }

  Varset FV = left->free_var(true);
  
  for (int i = 0; i < varlist.size(); i++) {
    // for every meta-variable Z:
    string pos = varlist[i];
    PVariable Z = dynamic_cast<MetaApplication*>(left->subterm(
      pos))->get_metavar();
    
    // if it doesn't have the same output type as the main
    // term, this isn't going to work anyway
    PType outp = Z->query_type();
    while (outp->query_composed()) outp = outp->query_child(1);
    PTerm l = left->copy();
    PTerm r = right->copy();
    TypeSubstitution theta;
    if (outp->query_typevar()) {
      theta[dynamic_cast<TypeVariable*>(outp)] = left->query_type()->copy();
    }
    else if (l->query_type()->query_typevar()) {
      theta[dynamic_cast<TypeVariable*>(left->query_type())] = outp->copy();
    }
    else if (!l->query_type()->equals(outp)) {
      delete l; delete r; continue;
    }
    l->apply_type_substitution(theta);
    r->apply_type_substitution(theta);
    Z->apply_type_substitution(theta);

    // find out whether it's ever applied to a list of other
    // meta-variables X1 ... Xn (in the right-hand side), and save
    // the variables if so; these Xi may only occur once in the
    // left-hand side and must all be different
    vector<PVariable> X;
    vector<string> Xposl;
    vector<string> Xposr;
    vector<string> Zpos = find_metavar(right, Z);
    int j;
    for (int n = 0; n < Zpos.size(); n++) {
      string pp = Zpos[n];
      bool ok = true;
      vector<PVariable> appliedto;
      // see whether this occurrence of Z has the form Z X1 ... Xn
      while (r->subterm(pp)->query_type()->query_composed()) {
        pp = pp.substr(0,pp.length()-1);
        PTerm sub = right->subterm(pp);
        if (!sub->query_application()) {ok = false; break;}
        PTerm at = sub->subterm("2");
        if (!at->query_meta()) {ok = false; break;}
        if (at->subterm("0") != NULL) {ok = false; break;}
        PVariable Y = dynamic_cast<MetaApplication*>(at)->get_metavar();
        if (!FV.contains(Y->query_index())) {ok = false; break;}
        appliedto.push_back(Y);
      }
      if (!ok) continue;
      // see whether each Xi only occurs once on the left
      vector<string> Xposltmp;
      for (j = 0; j < appliedto.size(); j++) {
        vector<string> leftpos = find_metavar(left, appliedto[j]);
        if (leftpos.size() != 1) {ok = false; break;}
        Xposltmp.push_back(leftpos[0]);
      }
      if (!ok) continue;
      // see whether the Xi are disjunct
      for (j = 1; j < appliedto.size() && ok; j++) {
        for (int k = 0; k < j && ok; k++) {
          if (appliedto[j]->equals(appliedto[k])) ok = false;
        }
      }
      if (!ok) continue;
      // looks like all is good - save the data!
      for (j = 0; j < appliedto.size(); j++) {
        X.push_back(appliedto[j]);
        Xposl.push_back(Xposltmp[j]);
        Xposr.push_back(pp);
      }
    }

    // if it's not, we can't construct the usual counterexample
    if (X.size() == 0) {delete l; delete r; continue;}
    
    // Z has to occur in an environment D[Z] which has the same
    // type as one of the Xi, which is disjunct from the
    // positions of all Xi in the same context, and which does
    // not freely contain variables which are bound in a superterm
    string Dpos = find_base_superterm(left, pos);
    if (Dpos == "") {delete l; delete r; continue;}

    for (int j = 0; j < X.size(); ) {
      int k = j+1;
      while (k < X.size() && Xposr[j] == Xposr[k]) k++;
      
      bool ok = true;
      int m;
      // check whether all Xi in this group occur at a different
      // position in l
      for (m = j; m < k && ok; m++) {
        if (Dpos == Xposl[j].substr(0,Dpos.length())) ok = false;
      }
      if (!ok) {j = k; continue;}
      // check whether any of the Xi has a matching type
      PType goal = left->subterm(Dpos)->query_type();
      int offender = -1;
      if (goal->query_typevar()) {
        TypeSubstitution subst;
        subst[dynamic_cast<TypeVariable*>(goal)] =
          X[j]->query_type()->copy();
        l->apply_type_substitution(subst);
        r->apply_type_substitution(subst);
        offender = j;
      }
      else {
        for (m = j; m < k && offender == -1; m++) {
          if (X[m]->query_type()->query_typevar()) {
            TypeSubstitution subst;
            subst[dynamic_cast<TypeVariable*>(X[m]->query_type())] =
              goal->copy();
            l->apply_type_substitution(subst);
            r->apply_type_substitution(subst);
            offender = m;
          }
          else if (X[m]->query_type()->equals(goal)) offender = m;
        }
      }
      
      // we have a counterexample for termination!
      if (offender != -1) {
        vector<string> lposY;
        for (m = j; m < k; m++) {
          lposY.push_back(Xposl[m]);
        }
        omega(left, right, Dpos, pos, Xposr[j], lposY, offender-j);
        delete l; delete left;
        delete r; delete right;
        return true;
      }
      
      j = k;
    }
    delete l; delete r;
  }
  delete left; delete right;

  return false;
}

vector<string> NonTerminator :: find_metavar(PTerm term, PVariable X) {
  vector<string> positions = term->query_positions();
  vector<string> ret;
  for (int i = 0; i < positions.size(); i++) {
    PTerm t = term->subterm(positions[i]);
    if (t->query_meta() &&
        dynamic_cast<MetaApplication*>(t)->get_metavar()->equals(X))
      ret.push_back(positions[i]);
  }
  return ret;
}

string NonTerminator :: find_base_superterm(PTerm term, string pos) {
  if (pos == "") return pos;
  pos = pos.substr(0,pos.length()-1);
  PTerm sub = term->subterm(pos);
  if (sub->query_type()->query_composed())
    return find_base_superterm(term, pos);
  else if (!sub->free_var().empty())
    return find_base_superterm(term, pos);
  return pos;
}

/**
 * given that l = C[D[X],Y1,...,Yn] and r has a subterm
 * X*Y1***Yn, that D[X] has the same (base) type as one of the Yi
 * (Y_{offender} to be precise), that basepos is the position of
 * D[X] in l, each lposY[i] is the position of Yi in l and rposapp
 * is the position of the application in r, determine a
 * counterexample for termination
 */
void NonTerminator :: omega(PTerm l, PTerm r, string basepos,
                            string lposX, string rposapp,
                            vector<string> lposY, int offender) {
  // make a vector of variables of the same types as the terms in
  // lposY
  vector<PVariable> y;
  int i;
  for (i = 0; i < lposY.size(); i++) {
    y.push_back(new Variable(l->subterm(lposY[i])->query_type()->copy()));
  }

  // build the term w = /\y1...yn.C[yi,y1,...,yn]
  PTerm w = l->copy();
  for (i = 0; i < y.size(); i++) {
    delete w->replace_subterm(y[i]->copy(), lposY[i]);
  }
  delete w->replace_subterm(y[offender]->copy(), basepos);
  for (i = y.size()-1; i >= 0; i--) {
    w = new Abstraction(y[i], w);
  }

  // make a term Omega = C[D[w],y1,...,D[w],...,yn]
  PTerm Omega = l->copy();
  PVariable Z = dynamic_cast<MetaApplication*>(
    l->subterm(lposX))->get_metavar();
  Substitution subst;
  subst[Z] = w;
  Omega = Omega->apply_substitution(subst);
  PTerm Do = Omega->subterm(basepos)->copy();
  for (i = 0; i < lposY.size(); i++) {
    Z = dynamic_cast<MetaApplication*>(
      l->subterm(lposY[i]))->get_metavar();
    if (i == offender) subst[Z] = Do;
    else subst[Z] = new Variable(y[i]->query_type()->copy());
  }
  Omega = Omega->apply_substitution(subst);
  
  // replace all type variables by something innocent
  Varset vars = Omega->free_typevar();
  TypeSubstitution ts;
  for (Varset::iterator it = vars.begin(); it != vars.end(); it++) {
    ts[*it] = new DataType("o");
  }
  Omega->apply_type_substitution(ts);

  // replace all meta-variables by variables of corresponding type
  Substitution subst2;
  vector<string> allpos = Omega->query_positions();
  for (i = 0; i < allpos.size(); i++) {
    PTerm lsub = Omega->subterm(allpos[i]);
    if (lsub->query_meta()) {
      PVariable mv = dynamic_cast<MetaApplication*>(lsub)->get_metavar();
      if (!subst2.contains(mv)) {
        PVariable v = new Variable(mv->query_type()->copy());
        subst2[mv] = v;
      }
    }
  }
  Omega = Omega->apply_substitution(subst2);

  // print the start of an infinite reduction!
  wout.print("This system is non-terminating, as demonstrated by "
    "the following reduction:\n");
  ArList arities = wout.arities_for_system(F, rules);
  map<int,string> metanaming, freenaming, boundnaming;
  wout.start_reduction(wout.print_term(Omega, arities, F, metanaming,
                                       freenaming, boundnaming));
  TypeSubstitution theta;
  Substitution gamma;
  if (!l->instantiate(Omega, theta, gamma)) {
    wout.end_reduction();
    wout.print("ERROR");
    return;
  }
  delete Omega;
  PTerm next = r->copy();
  next->apply_type_substitution(theta);
  next = next->apply_substitution(gamma);
  boundnaming.clear();
  wout.continue_reduction(wout.reduce_arrow(),
      wout.print_term(next, arities, F, metanaming, freenaming, boundnaming));
  next = beta->normalise(next);
  boundnaming.clear();
  wout.continue_reduction(wout.beta_arrow(),
      wout.print_term(next, arities, F, metanaming, freenaming, boundnaming));
  wout.end_reduction();
  if (rposapp == "") wout.print("That is, a term reduces to itself.");
  else wout.print("That is, a term s reduces to a term of the form "
    "C[s].");
}

vector<string> NonTerminator :: clean_metavariables(PTerm &left,
                                                    PTerm &right) {
  // 1. Find all positions in left with meta-variables
  vector<string> positions = left->query_positions();
  int i;

  vector<string> metapositions;
  for (i = 0; i < positions.size(); i++) {
    if (left->subterm(positions[i])->query_meta())
      metapositions.push_back(positions[i]);
  }

  // 2. filter out those which do not have functional type, and those
  //    which occur more than once
  for (i = 0; i < metapositions.size(); i++) {
    PTerm sub = left->subterm(metapositions[i]);
    bool ok = true;
    if (!sub->query_type()->query_composed() &&
        sub->subterm("0") == NULL) ok = false;
    else {
      PVariable Z = dynamic_cast<MetaApplication*>(sub)->get_metavar();
      if (find_metavar(left, Z).size() != 1) ok = false;
    }
    if (!ok) {
      metapositions[i] = metapositions[metapositions.size()-1];
      metapositions.pop_back();
      i--;
    }
  }

  // 3. fix those which occur in the form /\x1...xn.Z[x1,...,xn] and
  //    filter out those which don't occur in such a form
  for (i = 0; i < metapositions.size(); i++) {
    MetaApplication* meta =
      dynamic_cast<MetaApplication*>(left->subterm(metapositions[i]));
    if (meta->subterm("0") == NULL) continue; // already in good form
    
    // get all arguments
    vector<PVariable> args;
    int j;
    for (j = 0; ; j++) {
      char ps[] = "A"; ps[0] = '0'+j;
      PTerm argj = meta->subterm(ps);
      if (argj == NULL) break;
      if (!argj->query_variable()) {vector<string> ret; return ret;}
        // shouldn't happen!
      args.push_back(dynamic_cast<PVariable>(argj));
    }

    // check whether it's an abstraction of the right form above
    string pos = metapositions[i];
    if (pos.length() < args.size()) {vector<string> ret; return ret;}
      // shouldn't happen!
    bool ok = true;
    for (j = args.size()-1; j >= 0 && ok; j--) {
      pos = pos.substr(0,pos.length()-1);
      PTerm super = left->subterm(pos);
      if (!super->query_abstraction()) ok = false;
      else {
        Abstraction *asuper = dynamic_cast<Abstraction*>(super);
        if (!asuper->query_abstraction_variable()->equals(args[j]))
          ok = false;
      }
    }
    // if it's not, ignore this path
    if (!ok) {
      metapositions[i] = metapositions[metapositions.size()-1];
      metapositions.pop_back();
      i--;
      continue;
    }
    // it is! Replace it by its unparsed form
    PVariable metavar = meta->get_metavar();
    PVariable mv = dynamic_cast<PVariable>(metavar->copy());
    MetaApplication *ma = new MetaApplication(mv);
    Substitution subst;
    subst[metavar] = ma->copy();
    delete left->replace_subterm(ma, pos);
    right = right->apply_substitution(subst);
    metapositions[i] = pos;
  }

  return metapositions;
}
bool NonTerminator :: non_terminating() {
  // check 1: is the right-hand side of a rule instantiated in the left?
  wout.start_method("obvious loop");
  for (int i = 0; i < rules.size(); i++) {
    if (obvious_loop(rules[i])) {
      wout.succeed_method("obvious loop");
      return true;
    }
  }
  wout.abort_method("obvious loop");

  // check 2: is there a rule directly iplementing untyped lambda-calculus?
  wout.start_method("lambda calculus");
  for (int i = 0; i < rules.size(); i++) {
    if (lambda_calculus(rules[i])) {
      wout.succeed_method("lambda calculus");
      return true;
    }
  }
  wout.abort_method("lambda calculus");
  
  return false;
}

void NonTerminator :: possible_reductions(PTerm term, vector<Rule*>
                                          &rule, vector<string> &pos) {
  vector<string> positions = term->query_positions();
  for (int i = 0; i < positions.size(); i++) {
    for (int j = -1; j < int(rules.size()); j++) {
      Rule *attempt;
      if (j == -1) attempt = beta;
      else attempt = rules[j];
      if (attempt->applicable(term, positions[i])) {
        pos.push_back(positions[i]);
        rule.push_back(attempt);
      }
    }
  }
}

/*
void NonTerminator :: meta_reductions(PTerm term, vector<Rule*> &rule,
                                      vector<string> &pos) {
  // first find all unreliable positions
  vector<string> positions = term->query_positions();
  vector<string> unreliable;
  for (int i = 0; i < positions.size(); i++) {
    PTerm sub = term->subterm(positions[i]);
    if (sub->query_meta()) unreliable.push_back(positions[i]);
    else if (immediate_beta && sub->query_head()->query_meta())
      unreliable.push_back(positions[i]);
  }

  // then check on which positions term can be reduced
  possible_reductions(term, rule, pos);

  // and remove the unreliable ones!
  for (int j = 0; j < pos.size(); j++) {
    bool ok = true;
    for (int k = 0; k < unreliable.size() && ok; k++) {
      if (pos[j].substr(0,unreliable[k].length()) == unreliable[k])
        ok = false;
    }
    if (ok) continue;
    rule[j] = rule[rule.size()-1]; rule.pop_back();
    pos[j] = pos[pos.size()-1]; pos.pop_back();
    j--;
  }
}
*/

