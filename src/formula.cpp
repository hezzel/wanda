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

#include "formula.h"
#include <cstdlib>
#include <cstdio>
#include <iostream>

Vars vars;

/* =========================== Vars functionality ========================== */
/* the global set of all variables */

Vars :: Vars() {
  reset();
}

int Vars :: query_size() {
  return val.size();
}

Valuation Vars :: query_value(unsigned int index) {
  return val[index];
}

void Vars :: force_value(unsigned int index, Valuation value) {
  val[index] = value;
}

void Vars :: add_vars(unsigned int number) {
  val.resize(val.size() + number, UNKNOWN);
  descriptions.resize(descriptions.size() + number, "");
}

void Vars :: set_description(unsigned int index, string desc) {
  descriptions[index] = desc;
}

string Vars :: query_description(unsigned int index) {
  if (descriptions[index] != "") return descriptions[index];
  char tmp[10];
  sprintf(tmp, "X%d", index);
  return string(tmp);
}

bool Vars :: has_description(unsigned int index) {
  return descriptions[index] != "";
}

string Vars :: to_string() {
  string ret = "";
  for (int i = 0; i < val.size(); i++) {
    ret += query_description(i) + " : " + (val[i] == TRUE ? "true" :
                                           val[i] == FALSE ? "false" :
                                                             "unknown") + "\n";
  }
  return ret;
}

void Vars :: reset() {
  val.clear();
  descriptions.clear();
  add_vars(2);
  set_description(0, "False");
  set_description(1, "True");
  force_value(0, FALSE);
  force_value(1, TRUE);
}

void Vars :: reset_valuation() {
  for (int i = 2; i < val.size(); i++)
    force_value(i, UNKNOWN);
}

int Vars :: false_var() { return 0; }
int Vars :: true_var() { return 1; }

/* ======================= base Formula functionality ====================== */

Formula :: Formula() : changed(false) {}

PFormula Formula :: judge(PFormula original, PFormula current) {
  if (original != current) {
    current->set_changes();
    delete original;
  }
  return current;
}

PFormula Formula :: simplify_main() {
  return this;
}

// must be defined, but should never be called in the inherit!
PFormula Formula :: copy() {
  return NULL;
}

// must be defined, but should never be called in the inherit!
string Formula :: to_string(bool brackets) {
  return "";
}

bool Formula :: query_top() {return false;}
bool Formula :: query_bottom() {return false;}
bool Formula :: query_conjunction() {return false;}
bool Formula :: query_disjunction() {return false;}
bool Formula :: query_variable() {return false;}
bool Formula :: query_antivariable() {return false;}
bool Formula :: query_negation() {return false;}
bool Formula :: query_special(string description) {return false;}

PFormula Formula :: negate() {
  return new Not(copy());
}

PFormula Formula :: substitute_variable(int index, PFormula with) {
  return this;
}

int Formula :: rename_variables(vector<int> org, vector<int> rep) {
  return 0;
}

PFormula Formula :: simplify() {
  bool oldchanged = changed;
  PFormula ret = simplify_main();
  ret = judge(this, ret);
  ret->changed = oldchanged;
  return ret;
}

void Formula :: reset_changes() {
  changed = false;
}

void Formula :: set_changes() {
  changed = true;
}

bool Formula :: query_changes(bool reset) {
  bool ret = changed;
  if (reset) changed = false;
  return ret;
}

PFormula Formula :: conjunctive_form(int depth, And *head,
                                     bool negrel, Atom *atom) {
  return this;
}

PFormula Formula :: conjunctive_form() {
  PFormula ret = new And(this);
  return ret->conjunctive_form();
}

/* ======================= universal truth/falsehood ======================= */

Top :: Top() :Formula() {}

PFormula Top :: copy() {
  return new Top();
}

bool Top :: query_top() {
  return true;
}

string Top :: to_string(bool brackets) {
  return "T";
}

PFormula Top :: negate() {
  return new Bottom;
}

Bottom :: Bottom() :Formula() {}

PFormula Bottom :: copy() {
  return new Bottom();
}

bool Bottom :: query_bottom() {
  return true;
}

string Bottom :: to_string(bool brackets) {
  return "F";
}

PFormula Bottom :: negate() {
  return new Top;
}

/* ================== atoms (variables and antivariables) ================== */

unsigned int Atom :: query_index() {
  return index;
}

void Atom :: force(bool maketrue) {
  // SHOULD NEVER BE CALLED HERE
}

void Atom :: unforce() {
  vars.force_value(index, UNKNOWN);
}

PFormula Atom :: simplify_main() {
  if (vars.query_value(index) == TRUE) {
    if (query_variable()) return new Top; else return new Bottom;
  }
  if (vars.query_value(index) == FALSE) {
    if (query_variable()) return new Bottom; else return new Top;
  }
  return this;
}

int Atom :: rename_variables(vector<int> org, vector<int> rep) {
  if (org.size() == 0) return 0;

  // do a binary search  
  int a = 0, b = org.size()-1;
  while (b-a > 1) {
    int c = (a+b)/2;
    if (org[c] < index) a = c;
    else if (org[c] > index) b = c;
    else a = b = c;
  }

  if (index == org[a]) index = rep[a];
  else if (index == org[b]) index = rep[b];
  else return 0;
  return 1;
}

Var :: Var(unsigned int ind) {
  index = ind;
}

void Var :: force(bool maketrue) {
  vars.force_value(index, maketrue ? TRUE : FALSE);
}

PFormula Var :: copy() {
  PFormula ret = new Var(index);
  return ret;
}

bool Var :: query_variable() {
  return true;
}

string Var :: to_string(bool brackets) {
  return vars.query_description(index);
}

PFormula Var :: negate() {
  return new AntiVar(index);
}

PFormula Var :: substitute_variable(int ind, PFormula with) {
  if (index != ind) return this;
  delete this;
  return with->copy();
}

AntiVar :: AntiVar(unsigned int ind) {
  index = ind;
}

void AntiVar :: force(bool maketrue) {
  vars.force_value(index, maketrue ? FALSE : TRUE);
}

PFormula AntiVar :: copy() {
  PFormula ret = new AntiVar(index);
  return ret;
}

bool AntiVar :: query_antivariable() {
  return true;
}

string AntiVar ::to_string(bool brackets) {
  return "-" + vars.query_description(index);
}

PFormula AntiVar :: negate() {
  return new Var(index);
}

PFormula AntiVar :: substitute_variable(int ind, PFormula with) {
  if (ind != index) return this;
  delete this;
  return with->negate();
}

/* =============================== And and Or ============================== */

AndOr :: ~AndOr() {
  for (int i = 0; i < children.size(); i++) {
    if (children[i] != NULL) delete children[i];
  }
}

void AndOr :: add_child(PFormula child) {
  children.push_back(child);
}

int AndOr :: query_number_children() {
  return children.size();
}

PFormula AndOr :: query_child(int index) {
  if (index < 0 || index >= children.size()) return NULL;
  return children[index];
}

PFormula AndOr :: replace_child(int index, PFormula newchild) {
  if (index < 0 || index >= children.size()) return NULL;
  PFormula ret = children[index];
  children[index] = newchild;
  return ret;
}

bool AndOr :: same_kind(PFormula formula) {
  return false;
    // SHOULD NOT BE CALLED HERE
}

PFormula AndOr :: copy() {
  AndOr *ret;
  if (query_conjunction()) ret = new And();
  else ret = new Or();
  
  for (int i = 0; i < children.size(); i++)
    ret->add_child(children[i]->copy());
  return ret;
}

PFormula AndOr :: negate() {
  if (children.size() == 0) {
    if (query_conjunction()) return new Bottom;
    else return new Top;
  }
  if (children.size() == 1) {
    return children[0]->negate();
  }
  AndOr *form;
  if (query_conjunction()) form = new Or(); else form = new And();
  for (int i = 0; i < children.size(); i++) {
    form->add_child(children[i]->negate());
  }
  return form;
}

PFormula AndOr :: substitute_variable(int index, PFormula with) {
  for (int i = 0; i < children.size(); i++) {
    if (children[i] != NULL)
      children[i] = children[i]->substitute_variable(index, with);
  }
  return this;
}

int AndOr :: rename_variables(vector<int> org, vector<int> rep) {
  int ret = 0;
  for (int i = 0; i < children.size(); i++) {
    if (children[i] != NULL)
      ret += children[i]->rename_variables(org, rep);
  }
  return ret;
}

/* helping function for simplifying; this splits the children in two groups:
 * atoms and interesting terms.  The atoms placed in the atoms vector are
 * forced to hold in the case of And, or not hold in the case of Or.
 * Bottom/Top occurances are neither and are thus removed from the formula;
 * if Bottom occurs in an And or Top in an Or all atoms are unforced again
 * and the offending formula returned.  If no such thing happens, NULL is
 * returned.
 *
 * If any of the children is of the same kind as this term, it's flattened
 * out (so something of the form X R (Y R Z) becomes X R Y R Z).
 *
 * If NULL is returned, the children vector is cleared.
 * If anything else is returned, the children vector contains all children
 * that have not yet been freed.
 */
PFormula AndOr :: flatten_base(vector<Atom*> &atoms,
                               vector<PFormula> &others) {
  // determine top/bottom elements, push atoms to the top, flatten immediate
  // children if necessary
  for (int i = 0; i < children.size(); i++) {
    PFormula child = children[i];
    children[i] = NULL;
    // atom -> first try simplifying if we should
    if (child->query_variable() || child->query_antivariable()) {
      child = child->simplify();
    }
    // still an atom, put in the atoms list and force it
    if (child->query_variable() || child->query_antivariable()) {
      Atom *at = dynamic_cast<Atom*>(child);
      atoms.push_back(at);
      at->force(query_conjunction());
    }
    else if (child->query_top() || child->query_bottom()) {
      // it proves / disproves the And/Or!
      if ((child->query_bottom() && query_conjunction()) ||
          (child->query_top() && query_disjunction())
         ) {
        int j;
        for (j = 0; j < atoms.size(); j++) {
          atoms[j]->unforce();
          delete atoms[j];
        }
        for (j = 0; j < others.size(); j++) delete others[j];
        return child;
      }
      // if not, it doesn't have any effect
      else delete child;
    }
    // child of the same kind -> take it in
    else
    if (same_kind(child)) {
      AndOr *ch = dynamic_cast<AndOr*>(child);
      PFormula ret = ch->flatten_base(atoms, others);
      delete child;
      if (ret != NULL) return ret;
    }
    // something else - just remember it
    else others.push_back(child);
  }

  children.clear();
  return NULL;
}

PFormula AndOr :: simplify_main() {
  vector<Atom*> atoms;
  vector<PFormula> others;
  int bottop, i;
  
  /* first simplifying round: remove top/bottom occurances (return if
   * we encounter F /\ X or T \/ X), take atoms apart and flatten out
   * nesting
   */
  PFormula ret = flatten_base(atoms, others);
  if (ret != NULL) return ret;
  
  /* Second simplifying round: simplify the arguments under assumption the
   * atoms hold in case of And or don't hold in case of Or, and put them back
   * in children!  Remember if we found any new variables, for in that case we
   * have to re-simplify.
   */
  bool found_vars = false;
  bool result_known = false;

  children.insert(children.end(), atoms.begin(), atoms.end());
  // simplify the arguments and put them back into children
  for (i = 0; i < others.size(); i++) {

    // don't bother simplifying any further if we already know what to return
    // anyway (but push the child back so we can free it later)
    if (result_known) {children.push_back(others[i]); continue;}
    
    PFormula child = others[i]->simplify();
    found_vars |= child->query_variable() || child->query_antivariable();
    
    if (child->query_top()) {
      if (query_disjunction()) result_known = true;
      delete child;
    }
    else if (child->query_bottom()) {
      if (query_conjunction()) result_known = true;
      delete child;
    }
    else if (same_kind(child)) {
      AndOr *ch = dynamic_cast<AndOr*>(child);
      for (int j = 0; j < ch->children.size(); j++) {
        found_vars |= (ch->children[j]->query_variable() ||
                       ch->children[j]->query_antivariable());
        children.push_back(ch->children[j]);
      }
      ch->children.clear();
      delete ch;
    }
    else children.push_back(child);
  }
  
  // undo the assumptions
  for (i = 0; i < atoms.size(); i++) atoms[i]->unforce();

  /* phase 3: rounding up */
  if (result_known) {
    if (query_conjunction()) return new Bottom;
    else return new Top;
  }
  if (found_vars) return simplify_main();

  if (children.size() == 0) {
    if (query_conjunction()) return new Top;
    else return new Bottom;
  }
  if (children.size() == 1) {
    PFormula ret = children[0];
    children.clear();
    return ret;
  }
  return this;
}

// does the simplifying without any recursive calls
PFormula AndOr :: top_simplify() {
  vector<Atom*> atoms;
  vector<PFormula> others;
  int bottop, i;
  
  /* first simplifying round: remove top/bottom occurances (return if
   * we encounter F /\ X or T \/ X), take atoms apart and flatten out
   * nesting
   */
  PFormula ret = flatten_base(atoms, others);
  if (ret != NULL) return ret;
  
  /* we mostly skip the second round, because we shouldn't touch the
   * children, but we do have to bring the formula back into the
   * right shape
   */
  children.insert(children.end(), atoms.begin(), atoms.end());
  for (i = 0; i < others.size(); i++) {
    children.push_back(others[i]);
  }
  for (i = 0; i < atoms.size(); i++) atoms[i]->unforce();

  /* phase 3: rounding up */
  if (children.size() == 0) {
    if (query_conjunction()) return new Top;
    else return new Bottom;
  }
  if (children.size() == 1) {
    PFormula ret = children[0];
    children.clear();
    return ret;
  }
  return this;
}


And :: And(PFormula left, PFormula middle, PFormula right) {
  add_child(left);
  add_child(middle);
  add_child(right);
}

And :: And(PFormula left, PFormula right) {
  add_child(left);
  add_child(right);
}

And :: And(PFormula start) {
  add_child(start);
}

And :: And() {}

bool And :: query_conjunction() {
  return true;
}

bool And :: same_kind(PFormula formula) {
  return formula->query_conjunction();
}

string And :: to_string(bool brackets) {
  if (children.size() == 0) return "T";
  if (children.size() == 1) return children[0]->to_string(brackets);
  string ret = children[0]->to_string(true);
  for (int i = 1; i < children.size(); i++) {
    ret += " /\\ " + children[i]->to_string(true);
  }
  if (brackets) ret = "(" + ret + ")";
  return ret;
}

PFormula And :: conjunctive_form(int depth, And *top, bool negrel,
                                 Atom *atom) {
  // handle erroneous calling by returning NULL
  if (depth != 1 && depth != 2) return NULL;
  if (top == NULL) return NULL;
  if (depth == 1 && negrel) return NULL;

  if (depth == 1) {
    // called directly from the top
    for (int i = 0; i < children.size(); i++) top->add_child(children[i]);
    children.clear();
    delete this;
    return new Top();
  }

  // depth == 2 and we're in an x \/ (A /\ B /\ C) case
  if (atom != NULL && !negrel) {
    PFormula ret = children[0]->conjunctive_form(depth, top, negrel, atom);
    for (int i = 1; i < children.size(); i++) {
      top->add_child(new Or(atom->copy(), children[i]));
    }
    children.clear();
    delete this;
    return ret;
  }

  // depth = 2 and we're not in xuch a case
  unsigned int index = vars.query_size();
  vars.add_vars(1);
  Var *var = new Var(index);
  // for each child phi_i, add var => phi_i to the top
  for (int i = 0; i < children.size(); i++) {
    top->add_child(new Or(new AntiVar(index), children[i]));
  }
  if (negrel) {
    // also add: phi_1 /\ ... /\ phi_n => var
    Or* newchild = new Or(new Var(index));
    for (int i = 0; i < children.size(); i++)
      newchild->add_child(children[i]->negate());
    top->add_child(newchild);
  }
  children.clear();
  delete this;
  return var;
}

PFormula And :: conjunctive_form() {
  bool did_something = true;
  while (did_something) {
    did_something = false;
    for (int i = 0; i < children.size(); i++) {
      children[i] = children[i]->simplify();
      did_something |= children[i]->query_changes();
      PFormula cf = children[i]->conjunctive_form(1, this, false, NULL);
      if (cf != children[i]) did_something = true;
      else if (cf->query_changes()) did_something = true;
      children[i] = cf;
      if (children[i] == NULL) {
        delete this;
        return NULL;
      }
      if (children[i]->query_bottom()) {
        delete this;
        return new Bottom;
      }
      if (children[i]->query_top()) {
        delete children[i];
        children[i] = children[children.size()-1];
        children.pop_back();
        i--;
      }
    }
    changed |= did_something;
  }

  if (children.size() == 0) {
    delete this;
    return new Top;
  }
  if (children.size() == 1) {
    PFormula ret = children[0];
    children.clear();
    delete this;
    return ret;
  }
  return this;
}

Or :: Or(PFormula left, PFormula middle, PFormula right) {
  add_child(left);
  add_child(middle);
  add_child(right);
}

Or :: Or(PFormula left, PFormula right) {
  add_child(left);
  add_child(right);
}

Or :: Or(PFormula start) {
  add_child(start);
}

Or :: Or() {}

bool Or :: query_disjunction() {
  return true;
}

bool Or :: same_kind(PFormula formula) {
  return formula->query_disjunction();
}

string Or :: to_string(bool brackets) {
  if (children.size() == 0) return "F";
  if (children.size() == 1) return children[0]->to_string(brackets);
  string ret = children[0]->to_string(true);
  for (int i = 1; i < children.size(); i++) {
    ret += " \\/ " + children[i]->to_string(true);
  }
  if (brackets) ret = "(" + ret + ")";
  return ret;
}

PFormula Or :: conjunctive_form(int depth, And *top, bool negrel,
                                Atom *atom) {
  if (depth != 1 && depth != 2) return NULL;

  if (depth == 2) {
    unsigned int index = vars.query_size();
    Var *var = new Var(index);
    // var => phi
    top->add_child(new Or(new AntiVar(index), this));
    if (negrel) {
      // phi => var, that is, (-phi_1 /\ ... /\ -phi_n) \/ var,
      // that is, (-phi_1 \/ var) /\ ... /\ (-phi_n \/ var)
     for (int i = 0; i < children.size(); i++) {
       top->add_child(new Or(children[i]->negate(), new Var(index)));
     }
    }
    return var;
  }

  // depth == 1 - are we in the X \/ B with X an atom situation?
  if (children.size() == 2 && !negrel) {
    if (children[0]->query_variable() || children[0]->query_antivariable()) {
      if (!children[1]->query_variable() && !children[1]->query_antivariable()) {
        children[1] = children[1]->conjunctive_form(2, top, false,
                                dynamic_cast<Atom*>(children[0]));
        if (children[1] == NULL) { delete this; return NULL; }
        return simplify();
      }
    }
    else if (children[1]->query_variable() || children[1]->query_antivariable()) {
      children[0] = children[0]->conjunctive_form(2, top, false,
                              dynamic_cast<Atom*>(children[1]));
      if (children[0] == NULL) { delete this; return NULL; }
      return simplify();
    }
  }

  // depth == 1 - first turn all children into things that cannot be
  // converted (ideally top, bottom, variables or antivariables)
  for (int i = 0; i < children.size(); i++) {
    PFormula ch = children[i]->conjunctive_form(2, top, negrel, NULL);
    if (ch != children[i] || ch->query_changes()) changed = true;
    children[i] = ch;
    if (ch == NULL) {
      delete this;
      return NULL;
    }
    if (ch->query_top()) {
      delete this;
      return new Top;
    }
    if (ch->query_bottom()) {
      delete ch;
      children[i] = children[children.size()-1];
      children.pop_back();
      changed = true;
    }
  }

  /*
  // now all its children are unconvertable, sort atoms to check
  // whether they're all distinct
  for (int i = 0; i < children.size()-1; i++) {
    int k1 = -1, k2 = -1;
    k1 = dynamic_cast<Atom*>(children[i])->query_index();
    k2 = dynamic_cast<Atom*>(children[i+1])->query_index();

    if (k2 <= k1) changed = true;
    if (k2 < k1) {
      PFormula tmp = children[i];
      children[i] = children[i+1];
      children[i+1] = tmp;
    }
    if (k1 == k2) {
      if ((children[i]->query_variable() &&
           children[i+1]->query_variable()) ||
          (children[i]->query_antivariable() &&
           children[i+1]->query_antivariable())) {
        delete children[i+1];
        children[i+1] = children[children.size()-1];
        children.pop_back();
      }
      else {
        // one is X, the other -X!
        delete this;
        return new Top;
      }
    }
  }
  */

  if (children.size() == 0) return new Bottom;
  if (children.size() == 1) return children[0];
  return this;
}

Not :: Not(PFormula _child) :Formula(), child(_child) {}
Not :: ~Not() { if (child != NULL) delete child; }

bool Not :: query_negation() {
  return true;
}

PFormula Not :: copy() {
  PFormula ret = new Not(child->copy());
  return ret;
}

string Not :: to_string(bool brackets) {
  return "-" + child->to_string(true);
}

PFormula Not :: negate() {
  return child;
}

PFormula Not :: simplify_main() {
  PFormula ret = child->negate();
  child = NULL;
  return ret->simplify();
}

PFormula Not :: query_child() {
  return child;
}

