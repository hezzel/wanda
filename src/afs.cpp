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

#include "afs.h"
#include "outputmodule.h"
#include "substitution.h"
#include <cstdio>
#include <sstream>
#include <iostream>

void MonomorphicAFS :: setup(Alphabet &F, Environment &V,
            vector<PTerm> &left, vector<PTerm> &right, ArList &lst) {

  // copy the alphabet and assign arities
  vector<string> symbs = F.get_all();
  for (int i = 0; i < symbs.size(); i++) {
    Sigma.add(symbs[i], F.query_type(symbs[i])->copy());
    if (lst.find(symbs[i]) == lst.end()) arities[symbs[i]] = 0;
    else arities[symbs[i]] = lst[symbs[i]];
  }

  // copy the environment
  Varset vars = V.get_variables();
  for (Varset::iterator it = vars.begin(); it != vars.end(); it++) {
    string name = V.get_name(*it);
    PType type = V.get_type(*it);
    gamma.add(*it, type->copy(), name);
  }

  // just take over the left and rights
  for (int j = 0; j < left.size() && j < right.size(); j++) {
    lhs.push_back(left[j]);
    rhs.push_back(right[j]);
  }

  normalised = check_normal();
  normalising_took_effort = false;
}

MonomorphicAFS :: MonomorphicAFS(Alphabet &F, Environment &V,
            vector<PTerm> &left, vector<PTerm> &right, ArList &lst) {
  setup(F, V, left, right, lst);
}

MonomorphicAFS :: MonomorphicAFS(Alphabet &F, Environment &V,
                         vector<PTerm> &left, vector<PTerm> &right) {
  ArList lst;
  setup(F, V, left, right, lst);
}

MonomorphicAFS :: ~MonomorphicAFS() {
  for (int i = 0; i < lhs.size(); i++) {
    delete lhs[i];
    delete rhs[i];
  }
}

void MonomorphicAFS :: recalculate_arity() {
  if (!normalised) return;
  ArList oldars;
  
  // assign default (maximal) arities
  vector<string> symbs = Sigma.get_all();
  for (int i = 0; i < symbs.size(); i++) {
    PConstant f = Sigma.get(symbs[i]);
    oldars[symbs[i]] = arities[symbs[i]];
    arities[symbs[i]] = f->query_max_arity();
    delete f;
  }
  
  // make sure they respect the rules
  for (int j = 0; j < lhs.size(); j++) {
    adjust_arities(lhs[j]);
    adjust_arities(rhs[j]);
  }

  // if anything changed, report this!
  bool anything_changed = false;
  for (int i = 0; i < symbs.size(); i++) {
    if (oldars[symbs[i]] != arities[symbs[i]])
      anything_changed = true;
  }
  if (anything_changed) {
    if (normalising_took_effort)
      wout.print("Moreover, by the same paper ");
    else wout.print("By " + wout.cite("Kop11", "Theorem 5") + ", ");
    wout.print("we can alter the arity; we will use the highest "
      "possible arity.\n");
    normalising_took_effort = true;
      // we now include changing arities in this
  }
}

void MonomorphicAFS :: recalculate_arity_eta() {
  if (!normalised) return;

  // assign default (maximal) arities
  vector<string> symbs = Sigma.get_all();
  for (int i = 0; i < symbs.size(); i++) {
    PConstant f = Sigma.get(symbs[i]);
    arities[symbs[i]] = f->query_max_arity();
    delete f;
  }
  
  // make sure they respect the left-hand sides
  for (int j = 0; j < lhs.size(); j++) {
    adjust_arities(lhs[j]);
  }

  // change the right-hand sides so they respect arity too
  for (int k = 0; k < rhs.size(); k++) {
    PTerm respect = respect_arity(rhs[k]);
    rhs[k] = respect;
  }
}

void MonomorphicAFS :: adjust_arities(PTerm s) {
  if (!s->query_head()->query_constant()) {
    // just make sure the subterms satisfy arities
    if (s->number_children() >= 1) adjust_arities(s->get_child(0));
    if (s->number_children() >= 2) adjust_arities(s->get_child(1));
    return;
  }
  
  // it's headed by a constant
  vector<PTerm> parts = s->split();
  PConstant f = dynamic_cast<PConstant>(parts[0]);
  if (parts.size()-1 < arities[f->query_name()])
    arities[f->query_name()] = parts.size()-1;
  for (int i = 1; i < parts.size(); i++) {
    adjust_arities(parts[i]);
  }
}

PTerm MonomorphicAFS :: respect_arity(PTerm term, bool ignoretop) {
  if (!ignoretop) {
    // if this term has the form f t1 ... tn with n < arity(f),
    // then make the term /\x.f t1 ... tn x and make sure that
    // respects arity!
    vector<PTerm> split = term->split();
    if (split[0]->query_constant()) {
      int nargs = split.size()-1;
      int ar = arities[split[0]->to_string(false)];
      if (nargs < ar) {
        PType it = term->query_type()->query_child(0)->copy();
        PVariable x = new Variable(it);
        term = respect_arity(new Application(term, x->copy()));
        term = new Abstraction(x, term);
        return term;
      }
    }
  }

  if (term->query_abstraction()) {
    PTerm sub = respect_arity(term->get_child(0), false);
    term->replace_subterm(sub, "1");
  }

  else if (term->query_application()) {
    PTerm sub2 = respect_arity(term->subterm("2"), false);
    term->replace_subterm(sub2, "2");
    PTerm sub1 = respect_arity(term->subterm("1"), true);
    term->replace_subterm(sub2, "1");
  }
  
  return term;
}

void MonomorphicAFS :: to_afsm(Alphabet &F,
                                vector<MatchRule*> &rules) {
  // make sure default requirements are satisfied
  normalise_rules();

  // copy the alphabet
  vector<string> symbs = Sigma.get_all();
  for (int i = 0; i < symbs.size(); i++) {
    F.add(symbs[i], Sigma.query_type(symbs[i])->copy());
  }

  // create all the rules
  for (int j = 0; j < lhs.size(); j++) {
    Substitution sub;
    Varset FV = lhs[j]->free_var();
    for (Varset::iterator it = FV.begin(); it != FV.end(); it++) {
      PType type = gamma.get_type(*it)->copy();
      PVariable v = new Variable(type);
      MetaApplication *ma = new MetaApplication(v);
      sub[*it] = ma;
    }
    PTerm l = lhs[j]->copy()->apply_substitution(sub);
    PTerm r = rhs[j]->copy()->apply_substitution(sub);
    rules.push_back(new MatchRule(l,r));
  }

  if (normalising_took_effort) {
    wout.print("We now transform the resulting AFS into an AFSM by "
      "replacing all free variables by meta-variables (with arity "
      "0).  This leads to the following AFSM:\n");
    wout.print_system(Sigma, rules);
  }
  else {
    wout.print("This AFS is converted to an AFSM simply by replacing "
      "all free variables by meta-variables (with arity 0).\n\n");
  }
}

bool MonomorphicAFS :: trivially_terminating(string &reason) {
  if (lhs.size() == 0) {
    reason = "No rules, and typed lambda-calculus is terminating.";
    return true;
  }
  return false;
}

bool MonomorphicAFS :: trivially_nonterminating(string &reason) {
  for (int i = 0; i < lhs.size(); i++) {
    if (lhs[i]->query_variable()) {
      reason = "Left-hand side of a rule is a variable:\n  " +
               lhs[i]->to_string(gamma) + " => " +
               rhs[i]->to_string(gamma);
      return true;
    }
  }
  return false;
}

void MonomorphicAFS :: rename_variables() {
  Varset vars = gamma.get_variables();
  int i = 0;

  for (Varset::iterator it = vars.begin(); it != vars.end(); it++) {
    int k = *it;

    // it's not - pick a new name
    vector<string> goodnames;
    if (gamma.get_type(k)->query_composed()) {
      goodnames.push_back("F"); goodnames.push_back("Z");
      goodnames.push_back("G"); goodnames.push_back("H");
      goodnames.push_back("I"); goodnames.push_back("J");
    }
    else {
      goodnames.push_back("X"); goodnames.push_back("Y");
      goodnames.push_back("U"); goodnames.push_back("V");
      goodnames.push_back("W"); goodnames.push_back("P");
    }

    // choose a start letter, and append more as necessary
    for ( ; i < 100000; i++) {
      string head = goodnames[i%6];
      char aa[6]; sprintf(aa, "%d", int(i/6));
      string name = aa;
      if (name == "0") name = "";
      name = head + name;
      if (!Sigma.contains(name)) {
        gamma.rename(k, name);
        i++;
        break;
      }
    }
  }
}

string MonomorphicAFS :: to_string() {
  string ret;
  int i;

  // alphabet
  vector<string> symbols = Sigma.get_all();
  for (i = 0; i < symbols.size(); i++) {
    ret += symbols[i] + " : " +
           Sigma.query_type(symbols[i])->to_string() + "\n";
  }
  ret += "\n";

  // environment
  rename_variables();
  Varset vars = gamma.get_variables();
  for (Varset::iterator it = vars.begin(); it != vars.end(); it++) {
    ret += gamma.get_name(*it) + " : " +
           gamma.get_type(*it)->to_string() + "\n";
  }
  ret += "\n";

  // rules
  for (i = 0; i < lhs.size(); i++) {
    ret += lhs[i]->to_string(gamma) + " => " + rhs[i]->to_string(gamma) + "\n";
  }
  return ret;
}

void MonomorphicAFS :: pretty_print() {
  wout.start_box();
  wout.print_header("Alphabet:");
  wout.print_alphabet(Sigma, arities);
  wout.print_header("Rules:");

  wout.start_table();

  for (int i = 0; i < lhs.size(); i++) {
    map<int,string> metanaming, freenaming, boundnaming;
    vector<string> entry;
    entry.push_back(wout.print_term(lhs[i], arities, Sigma,
                                    metanaming, freenaming,
                                    boundnaming));
    entry.push_back(wout.rule_arrow());
    entry.push_back(wout.print_term(rhs[i], arities, Sigma,
                                    metanaming, freenaming,
                                    boundnaming));
    wout.table_entry(entry);
  }

  wout.end_table();
  wout.end_box();
}

/* =============================================================== */
/* Normalising the rules:
 * 1) the left hand side should not contain free variables at the
 *    head of an application, and the left-hand side should be
 *    beta-normal.
 * 2) the rules should always be headed by a function symbol
 *
 * We can enforce part 1) by replacing applications s*t by a term
 * @*s*t, where @ is a new function symbol.  However, doing so has
 * the potential to make the system rather more difficult to manage.
 *
 * Therefore we do the following:
 * - in the left-hand side, whenever a free variable X occurs at the
 *   head of a term, and any f has the same output type, make sure
 *   there is a rule with X replaced by f * Y1 *** Yn; this does not
 *   affect the reduction relation in any way
 *     now those head variables can be assumed to only match
 *     abstractions and variables
 * - save all types of leading free variables and abstractions in the
 *   left-hand side as "dangerous";
 *     applications with a non-functional head should be marked with
 *     an @
 * - in the left-hand side, mark all leading free variables and
 *   abstractions with @
 * - in the right-hand side, replace all applications s*t with s
 *   having a dangerous type and not headed by a function symbol, by
 *   @*s*t
 * - add rules @*X*Y => X*Y for all newly introduced @
 *     adding these @ symbols with rules does not affect termination;
 *     they could equally be replaced by a lambda-term
 *
 * Now we have point 1.  To enforce point 2, if there is any rule
 * whose lhs is a single variable, just delete all other rules (that
 * rule will always lead to non-termination).  If there is a rule
 * whose lhs is an abstraction, introduce a symbol $2 and replace all
 * abstractions in either side which have the same type by $2*s.
 */
/* =============================================================== */

bool MonomorphicAFS :: check_normal() {
  for (int i = 0; i < lhs.size(); i++) {
    Varset FV = lhs[i]->free_var();
    vector<string> lpos = lhs[i]->query_positions();
    vector<string> rpos = rhs[i]->query_positions();

    // the left-hand side may not be an abstraction
    if (lhs[i]->query_abstraction()) return false;

    // the left-hand side may not contain applications headed by
    // either a variable or an abstraction
    for (int j = 0; j < lpos.size(); j++) {
      PTerm sub = lhs[i]->subterm(lpos[j]);
      if (!sub->query_application()) continue;
      PTerm head = sub->get_child(0);
      if (head->query_abstraction()) return false;
      if (head->query_variable() &&
          FV.contains(dynamic_cast<PVariable>(head))) return false;
    }

/* // this is allowed now!

    // the right-hand side may not contain applications headed by
    // an abstraction
    for (int k = 0; k < rpos.size(); k++) {
      PTerm sub = rhs[i]->subterm(rpos[k]);
      if (sub->query_application() &&
          sub->get_child(0)->query_abstraction()) return false;
    }
*/
  }

  return true;
}

vector<PVariable> MonomorphicAFS :: find_free_head_variables(PTerm l,
                                                     Varset &bound) {
  
  vector<PVariable> vars;

  // not an application, and no subterms
  if (l->query_variable() || l->query_constant()) return vars;

  // abstraction => check subterms, but exclude the binder
  if (l->query_abstraction()) {
    Abstraction *abs = dynamic_cast<Abstraction*>(l);
    bound.add(abs->query_abstraction_variable());
    vars = find_free_head_variables(l->get_child(0), bound);
    bound.remove(abs->query_abstraction_variable());
    return vars;
  }

  if (!l->query_application()) return vars; // err

  // it's an application - check subterms, and add the head if that's
  // a variable
  vars = find_free_head_variables(l->get_child(0), bound);
  vector<PVariable> vars2 =
    find_free_head_variables(l->get_child(1), bound);
  vars.insert(vars.end(), vars2.begin(), vars2.end());
  if (l->get_child(0)->query_variable()) {
    PVariable v = dynamic_cast<PVariable>(l->get_child(0));
    if (!bound.contains(v)) vars.push_back(v);
  }
  return vars;
}

void MonomorphicAFS :: mark_leading_abstraction_types(PTerm s) {
  
  if (s->query_abstraction())
    mark_leading_abstraction_types(s->subterm("1"));

  if (s->query_application()) {
    mark_leading_abstraction_types(s->subterm("1"));
    mark_leading_abstraction_types(s->subterm("2"));
    if (s->subterm("1")->query_abstraction()) {
      PType type = s->subterm("1")->query_type();
      if (!query_dangerous(type))
        dangerous_types.push_back(type->copy());
    }
  }
}

bool MonomorphicAFS :: query_dangerous(PType type) {
  for (int i = 0; i < dangerous_types.size(); i++) {
    if (dangerous_types[i]->equals(type)) return true;
  }
  return false;
}

bool MonomorphicAFS :: rule_exists(PTerm left, PTerm right) {
  Environment delta;
  string srule = left->to_string(delta) + " => " +
                 right->to_string(delta);
  for (int i = 0; i < lhs.size(); i++) {
    Environment chi;
    string arule = lhs[i]->to_string(chi) + " => " +
                   rhs[i]->to_string(chi);
    if (srule == arule) return true;
  }
  return false;
}

int MonomorphicAFS :: maximal_left_arity(string f) {
  int top = arities[f];
  for (int i = 0; i < lhs.size(); i++) {
    vector<PTerm> queue;
    queue.push_back(lhs[i]);
    for (int j = 0; j < queue.size(); j++) {
      PTerm s = queue[j];

      // if it's an application f s1 ... sk, find k and see whether
      // it's larger than what we had so far; also add each si to the
      // queue for further checking
      if (s->query_application() &&
          s->query_head()->to_string(false,false) == f) {
        int k = 0;
        while (s->query_application()) {
          queue.push_back(s->get_child(1));
          s = s->get_child(0);
          k++;
        }
        if (k > top) top = k;
      }
      // for anything else, just add the children to the checking queue
      else {
        for (int k = 0; k < s->number_children(); k++) {
          queue.push_back(s->get_child(k));
        }
      }
    }
  }
  return top;
}

vector<string> MonomorphicAFS :: constants_with_type(PType output,
                                                     map<string,int> &maxar) {
  vector<string> ret;

  vector<string> symbols = Sigma.get_all();
  for (int i = 0; i < symbols.size(); i++) {
    string f = symbols[i];

    // we seek to keep occurrences of f X1 ... Xk intact, where k is
    // the greatest number such that f occurs with k arguments in the
    // left-hand side; for any application of f on more than k
    // arguments, the AP symbol may be used
    maxar[f] = maximal_left_arity(f);

    // if f X1 ... Xi has an output type output -- with i < maxar --
    // then we return f, since we should not split up f X1 ... Xi
    PType ot = Sigma.query_type(f);
    int k;
    for (k = 0; k < arities[f]; k++) ot = ot->query_child(1);
    for (; k < maxar[f]; k++) {
      if (ot->equals(output)) ret.push_back(f);
      ot = ot->query_child(1);
    }
  }

  return ret;
}

map<string,int> MonomorphicAFS :: instantiate_head_variables() {
  map<string,int> maxar;
  for (int i = 0; i < lhs.size(); i++) {
    PTerm l = lhs[i], r = rhs[i];
    Varset bound;
    vector<PVariable> vars = find_free_head_variables(l,bound);
    for (int j = 0; j < vars.size(); j++) {
      PVariable v = vars[j];
      PType type = v->query_type();
      vector<string> fillin = constants_with_type(type, maxar);
      for (int k = 0; k < fillin.size(); k++) {
        string f = fillin[k];

        // we're going to add a rule with X replaced by f X1 ... Xn
        PTerm newleft = l->copy();
        PTerm newright = r->copy();
        PTerm replacement = Sigma.get(f);
        while (!replacement->query_type()->equals(type)) {
          PType nt = replacement->query_type()->query_child(0)->copy();
          PVariable nv = new Variable(nt);
          gamma.add(nv);
          replacement = new Application(replacement, nv);
        }
        Substitution sub;
        sub[v] = replacement;
        newleft = newleft->apply_substitution(sub);
        newright = newright->apply_substitution(sub);
        if (rule_exists(newleft, newright)) {
          delete newleft;
          delete newright;
        }
        else {
          lhs.push_back(newleft);
          rhs.push_back(newright);
        }
      }
    }
  }
  return maxar;
}

PConstant MonomorphicAFS :: at(PType type) {
  string tname = type->to_string();
  if (ats[tname] == "") {
    // create a new name for it, within the naming constraints
    char num[10];
    sprintf(num, "~AP%d", ats.size());
    ats[tname] = num;

    // add the symbol to the alphabet
    PType ctype = new ComposedType(type->copy(), type->copy());
    Sigma.add(num, ctype);

    // create a rule for it
    PVariable x = new Variable(type->copy());
    PVariable y = new Variable(type->query_child(0)->copy());
    gamma.add(x);
    gamma.add(y);
    PTerm left = new Application(new Application(
                                 Sigma.get(ats[tname]), x), y);
    lhs.push_back(left);
    PTerm right = new Application(x->copy(), y->copy());
    rhs.push_back(right);
  }

  return Sigma.get(ats[tname]);
}

PConstant MonomorphicAFS :: lam(PType type) {
  string tname = type->to_string();
  if (lams[tname] == "") {
    // create a new name for it, within the naming constraints
    char num[10];
    sprintf(num, "~L%d", lams.size());
    lams[tname] = num;

    // add the symbol to the alphabet
    PType ctype = new ComposedType(type->copy(), type->copy());
    Sigma.add(num, ctype);

    // create a rule for it
    PVariable x = new Variable(type->copy());
    gamma.add(x);
    lhs.push_back(new Application(Sigma.get(lams[tname]), x));
    rhs.push_back(x->copy());
  }

  return Sigma.get(lams[tname]);
}

PTerm MonomorphicAFS :: add_at(PTerm term, map<string,int> &maxar) {
  if (term->query_abstraction()) {
    term->replace_subterm(add_at(term->subterm("1"), maxar), "1");
    return term;
  }

  if (!term->query_application()) return term;

  PTerm s = term->subterm("1");
  PTerm t = term->subterm("2");

  bool needs_at = false;
  if (query_dangerous(s->query_type())) {
    PTerm head = s->query_head();
    if (!head->query_constant()) needs_at = true;
    else {
      int k = maxar[head->to_string(false, false)];
      PTerm tmp = s;
      for (PTerm tmp = s; tmp->query_application();
           tmp = tmp->get_child(0)) k--;
      if (k <= 0) needs_at = true;
    }
  }

  if (!needs_at) {
    s = add_at(s, maxar);
    t = add_at(t, maxar);
    term->replace_subterm(s, "1");
    term->replace_subterm(t, "2");
    return term;
  }

  term->replace_subterm(NULL, "1");
  term->replace_subterm(NULL, "2");
  delete term;
  s = add_at(s, maxar);
  t = add_at(t, maxar);
  PTerm ret = at(s->query_type());
  ret = new Application(ret, s);
  ret = new Application(ret, t);
  return ret;
}

PTerm MonomorphicAFS :: add_lam(PTerm term) {
  if (term->query_abstraction()) {
    term->replace_subterm(add_lam(term->subterm("1")), "1");
    if (query_dangerous(term->query_type())) {
      return new Application(lam(term->query_type()), term);
    }
    else return term;
  }

  if (term->query_application()) {
    term->replace_subterm(add_lam(term->subterm("1")), "1");
    term->replace_subterm(add_lam(term->subterm("2")), "2");
  }

  return term;
}

void MonomorphicAFS :: normalise_rules() {
  int i;

  if (normalised) return;

  /* Step 1: determine all types such that the left-hand side
   * contains an application s * t, where s is a free variable.
   * These types are considered "dangerous".
   */
  for (i = 0; i < lhs.size(); i++) {
    PTerm l = lhs[i];
    // find the free variables which occur at a head
    Varset bound;
    vector<PVariable> vars = find_free_head_variables(l, bound);
    for (int j = 0; j < vars.size(); j++) {
      if (!query_dangerous(vars[j]->query_type())) {
        dangerous_types.push_back(vars[j]->query_type()->copy());
      }
    }
  }

  /* Step 2: for each rule, for each head variable X occuring in its
   * left hand side, for each output type corresponding with the type
   * of X, add a rule with X replaced by f Y1 ... Yn (fresh Y1,...,
   * Yn).
   */
  map<string,int> maxar = instantiate_head_variables();

  /* Step 3: replace non-functional applications of a dangerous type
   * by a function application (~AP a b instead of a b).
   */
  for (i = 0; i < lhs.size(); i++) {
    if (lhs[i]->query_head()->to_string(false).substr(0,3) == "~AP") continue;
    PTerm l = add_at(lhs[i], maxar);
    PTerm r = add_at(rhs[i], maxar);
    lhs[i] = l;
    rhs[i] = r;
  }

  /* intermediate: clear dangerous_types, because we'll reuse it for
   * abstractions
   */
  for (i = 0; i < dangerous_types.size(); i++)
    delete dangerous_types[i];
  dangerous_types.clear();

  /* Step 4: make a new list of dangerous types: the types of all
   * abstractions occurring at the head of an application, and of
   * all left-hand sides which are abstractions.
   */
  for (i = 0; i < lhs.size(); i++) {
    mark_leading_abstraction_types(lhs[i]);
    //mark_leading_abstraction_types(rhs[i]);
      // this will remove beta-redexes in the right-hand side; we're
      // not doing this because WANDA can handle non-beta-normal
      // right-hand sides!
    if (lhs[i]->query_abstraction()) {
      PType type = lhs[i]->query_type();
      if (!query_dangerous(type))
        dangerous_types.push_back(type->copy());
    }
  }

  /* Step 5: add a ^ symbol around all abstractions of dangerous
   * type.
   */
  for (i = 0; i < lhs.size(); i++) {
    PTerm l = add_lam(lhs[i]);
    PTerm r = add_lam(rhs[i]);
    lhs[i] = l;
    rhs[i] = r;
  }

  for (i = 0; i < dangerous_types.size(); i++)
    delete dangerous_types[i];
  dangerous_types.clear();

  normalised = true;
  normalising_took_effort = true;

  wout.print("Using the transformations described in " +
    wout.cite("Kop11") + ", this system can be brought in a " +
    "form without leading free variables in the left-hand side, " +
    "and where the left-hand side of a variable is always a "
    "functional term or application headed by a functional term.\n");
}

#if 0
/* the following three functions implement the eta shorten hack
 * which was removed because it's extremely hacky, and doesn't
 * actually seem to help much
 */

// returns whether the given term is linear and the variables in
// encountered do not occur in it (the variables in bound are
// excluded)
bool MonomorphicAFS :: linear(PTerm l, Varset &bound,
                              Varset &encountered) {
  if (l->query_variable()) {
    int x = dynamic_cast<PVariable>(l)->query_index();
    if (bound.contains(x)) return true;
    if (encountered.contains(x)) return false;
    encountered.add(x);
    return true;
  }

  if (l->query_abstraction()) {
    Abstraction *abs = dynamic_cast<Abstraction*>(l);
    int x = abs->query_abstraction_variable()->query_index();
    if (bound.contains(x)) x = -1;  // shouldn't happen!
    bound.add(x);
    bool ret = linear(l->subterm("1"), bound, encountered);
    bound.remove(x);
    return ret;
  }

  if (l->query_application()) {
    return linear(l->subterm("1"), bound, encountered) &&
           linear(l->subterm("2"), bound, encountered);
  }

  return true;
}

// if term is the eta-long form of a variable, returns that variable,
// otherwise returns NULL
PVariable MonomorphicAFS :: eta_long_variable(PTerm term, bool strict) {
  if (strict && !term->query_abstraction()) return NULL;

  // if term is /\x1...xn.s, split in [x1,...,xn] and s
  vector<PVariable> bound;
  while (term->query_abstraction()) {
    Abstraction *abs = dynamic_cast<Abstraction*>(term);
    PVariable v = abs->query_abstraction_variable();
    bound.push_back(v);
    term = term->subterm("1");
  }

  // see whether s is, indeed, Z x1| ... xn|
  while (bound.size() > 0) {
    if (!term->query_application()) return NULL;
    PTerm appliedon = eta_long_variable(term->subterm("2"), false);
    if (appliedon == NULL) return NULL;
    if (!appliedon->equals(bound[bound.size()-1])) return NULL;
    term = term->subterm("1");
    bound.pop_back();
  }

  // is Z actually a variable? If so, return it!
  if (term->query_variable()) return dynamic_cast<PVariable>(term);
  else return NULL;
}

// to aid the weakness condition on stupid input, if a system
// is left-linear, but has /\x.Z x in the left-hand side,
// replaces this by just Z (this can be done without losing
// non-termination)
void MonomorphicAFS :: eta_shorten_hack() {
  int i;
  if (status != 0 && status != 5) return;

  // to avoid problems with weak systems, subterms /\x.Z x in the
  // left-hand side of a rule are replaced by just Z
  // this loses termination, so we'll only do it on left-linear
  // systems; this also avoids complications if that same Z occurs
  // elsewhere not inside an abstraction
  for (i = 0; i < afsrules.size(); i++) {
    Varset bound, encountered;
    if (!linear(afsrules[i]->left, bound, encountered)) return;
  }

  // make the change!
  for (i = 0; i < afsrules.size(); i++) {
    vector<PTerm> subs;
    subs.push_back(afsrules[i]->left);
    while (subs.size() != 0) {
      PTerm ss = subs[subs.size()-1];
      subs.pop_back();
      PVariable var;
      if (ss->query_abstraction()) {
        var = eta_long_variable(ss->subterm("1"), true);
        if (var == NULL) subs.push_back(ss->subterm("1"));
        else {
          delete ss->replace_subterm(var->copy(), "1");
          status = 5;
        }
      }
      if (ss->query_application()) {
        var = eta_long_variable(ss->subterm("2"), true);
        if (var == NULL) subs.push_back(ss->subterm("2"));
        else {
          delete ss->replace_subterm(var->copy(), "2");
          status = 5;
        }
        subs.push_back(ss->subterm("1"));
      }
    }
  }

  if (status == 5) cout << "Made a change!" << endl;
}
#endif

