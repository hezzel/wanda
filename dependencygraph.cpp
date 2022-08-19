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

#include "dependencygraph.h"
#include "outputmodule.h"
#include "substitution.h"
#include <cstdio>

DependencyGraph :: DependencyGraph(Alphabet &Sigma, DPSet &P,
                                   vector<MatchRule*> &R)
    :rules(R), pairs(P) {
  
  int i, j;

  get_eating_info(Sigma);
  get_reduction_info(Sigma);

  // initialise the graph, and calculate all connections
  for (i = 0; i < P.size(); i++) {
    graph_entry dummy;
    for (j = 0; j < P.size(); j++) {
      dummy.push_back(connection_possible(P[i], P[j]));
    }
    graph.push_back(dummy);
  }

  // and determine the reachability array
  calculate_reachable();
}

/* ================= generally helpful functions ================= */

bool DependencyGraph :: is_constructor(PTerm symbol) {
  // only function symbols are constructors
  if (!symbol->query_constant()) return false;
  // if it's the leading symbol of any rule, it's not a constructor
  string symbname = symbol->to_string(false);
  for (int i = 0; i < rules.size(); i++) {
    string headname =
      rules[i]->query_left_side()->query_head()->to_string(false);
    if (symbname == headname) return false;
  }
  // looks good!
  return true;
}

/* ==================== estimating the graph ===================== */

bool DependencyGraph :: connection_possible(DependencyPair *pair1,
                                            DependencyPair *pair2) {

  if (pair1 == NULL || pair2 == NULL) return false;
  
  PTerm from = pair1->query_right();
  PTerm to = pair2->query_left();

  // using a subterm step, things of the form Z[r1,...,rn]...rk
  // can reduce to anything
  if (from->query_head()->query_meta()) return true;

  // if from is not headed by a meta-application, we need an "inn"
  // reduction of a functional term (so only reducing arguments)
  vector<PTerm> splitfrom = from->split();
  vector<PTerm> splitto = to->split();
  if (splitfrom.size() > splitto.size() && !pair2->query_headmost())
    return false;
  if (splitfrom.size() < splitto.size() && !pair1->query_headmost())
    return false;
  int n = splitfrom.size(); if (splitto.size() < n) n = splitto.size();
  
  // head must be the same constant, with unifiable types
  if (!splitfrom[0]->query_constant() ||
      !splitto[0]->query_constant()) return true; // ERROR!
  PConstant f = dynamic_cast<PConstant>(splitfrom[0]);
  PConstant g = dynamic_cast<PConstant>(splitto[0]);
  if (f->query_name() != g->query_name()) return false;
  PType combination = typer.unify(f->query_type(), g->query_type());
  if (combination == NULL) return false;
  delete combination;

  // without a variable at the head, a reduction does not change type;
  // even if one of them is a headmost dependency pair this is the
  // case, because then separate pairs l X -> r X were added unless
  // it has a variable type, which unifies with everything
  combination = typer.unify(from->query_type(), to->query_type());
  if (combination == NULL) return false;
  delete combination;

  // finally, check whether the arguments can actually reduce!
  for (int i = 1; i < n; i++) {
    if (!reduction_possible(splitfrom[i], splitto[i], pair2))
      return false;
  }

  return true;
}

bool DependencyGraph :: reduction_possible(PTerm from, PTerm to,
                                           DependencyPair *todp) {
  
  // for a reduction to be possible, types must correspond
  PType type1 = from->query_type();
  PType type2 = to->query_type();
  PType combination = typer.unify(type1, type2);
  if (combination == NULL) return false;
  delete combination;

  // all variables occurring freely in to (but not inside a
  // meta-application, unless that position of the meta-application
  // is specified as non-eating), also occur in from
  Varset into = get_certain_variables(to, todp);
  Varset infrom = from->free_var();
  if (!infrom.contains(into)) return false;

  // s --> Z[x1,...,xn] if for all variables other than x1,...,xn,
  // s can reduce them away
  if (to->query_meta()) {
    // find all variables which should be reduced away
    Varset problems = from->free_var();
    for (int i = 0; i < to->number_children(); i++) {
      PTerm sub = to->get_child(i);
      if (!sub->query_variable()) return true;  // ergh
      problems.remove(dynamic_cast<PVariable>(sub));
    }
    // check whether they can be
    for (Varset::iterator it = problems.begin();
                          it != problems.end(); it++) {
      if (at_non_eating_pos(from, *it)) return false;
    }
    // looks okay!
    return true;
  }

  // /\x.s -->> /\x.t only if s --> t
  if (from->query_abstraction()) {
    if (!to->query_abstraction()) return false;
    Abstraction *fabs = dynamic_cast<Abstraction*>(from);
    Abstraction *tabs = dynamic_cast<Abstraction*>(to);
    PVariable fabsx = fabs->query_abstraction_variable();
    PVariable tabsx = tabs->query_abstraction_variable();
    PVariable newvar = new Variable(tabsx->query_type()->copy(),
                                    fabsx->query_index());
    Substitution sub;
    sub[tabs->query_abstraction_variable()] = newvar;
    PTerm test = tabs->subterm("1")->copy()->apply_substitution(sub);
    bool ret = reduction_possible(from->subterm("1"), test, todp);
    delete test;
    return ret;
  }

  // if left is a single variable, then right should be the same
  if (from->query_variable()) {
    if (!to->query_variable()) return false;
    return dynamic_cast<PVariable>(from)->query_index() !=
           dynamic_cast<PVariable>(to)->query_index();
  }
  // if left is a single constructor, then right should be the same
  if (is_constructor(from)) {
    if (!to->query_constant()) return false;
    return from->to_string(false) == to->to_string(false);
  }

  // left is an application, or a defined symbol - check the head
  PTerm fhead = from->query_head();
  PTerm thead = to->query_head();

  // check for things which shouldn't happen
  if (fhead->query_abstraction()) return true;
  if (thead->query_meta()) return true;

  // Z[s1,...,sn]...sk --> t is always possible, if the variable
  // restriction is satisfied
  if (fhead->query_meta()) return true;

  // if from is headed by a constructor or variable, to must have
  // the same form, so from is a l1 ... ln and to is a r1 ... rn
  if (fhead->query_variable() || is_constructor(fhead)) {
    if (!to->query_application()) return false;
    return reduction_possible(from->subterm("1"), to->subterm("1"),
                              todp) &&
           reduction_possible(from->subterm("2"), to->subterm("2"),
                              todp);
  }

  if (!fhead->query_constant()) return true;   // shouldn't happen!
  PConstant f = dynamic_cast<PConstant>(fhead);

  // if from is headed by a defined symbol, see whether it could
  // reduce to something of the form of to
  if (to->query_abstraction())
    return can_reduce_to[f->query_name() + " : #ABS"];
  if (thead->query_variable())
    return can_reduce_to[f->query_name() + " : #VAR"];
  if (thead->query_constant())
    return can_reduce_to[f->query_name() + " : " + thead->to_string(false)];

  // this probably shouldn't happen
  return true;
}

Varset DependencyGraph :: get_certain_variables(PTerm term,
                                                DependencyPair *dp) {
  Varset ret;

  // term = x => return { x }
  if (term->query_variable()) {
    ret.add(dynamic_cast<PVariable>(term));
    return ret;
  }

  // term = Z[s1,...,sn] => return U { gcv(si) | Z non-eating in i }
  if (term->query_meta()) {
    PVariable metavar =
      dynamic_cast<MetaApplication*>(term)->get_metavar();
    for (int i = 0; i < term->number_children(); i++) {
      PTerm child = term->get_child(i);
      if (child == NULL) break;
      if (dp->query_noneating(metavar->query_index(), i)) {
        Varset childvars = get_certain_variables(child, dp);
        ret.add(childvars);
      }
    }
    return ret;
  }

  // term = f => return emptyset
  if (term->query_constant()) return ret;

  // term = s t => return gcv(s) U gcv(t)
  if (term->query_application()) {
    ret = get_certain_variables(term->subterm("1"), dp);
    Varset ret2 =get_certain_variables(term->subterm("2"), dp);
    ret.add(ret2);
    return ret;
  }

  // errrrrr
  return ret;
}

/**
 * fill noneatingpos by the following rules:
 * - assume noneatingpos[f][k] is false for all defined symbols if
 *   k is more than the arity of f (running assumption)
 * - if we can see that a symbol could potentially destroy a variable
 *   occurring in position k, then set noneatingpos[f][k] to false
 *   (this uses the knowledge of things previously set to false)
 * - if we cannot see any further eating, then all remaining symbols
 *   are noneating!
 */
void DependencyGraph :: get_eating_info(Alphabet &Sigma) {
  int i;

  // step 1: initialise noneatingpos by setting all to true
  vector<string> names = Sigma.get_all();
  for (i = 0; i < names.size(); i++) {
    PConstant f = Sigma.get(names[i]);
    int n = f->query_max_arity();
    delete f;
    noneatingpos[names[i]].resize(n);
    for (int j = 0; j < n; j++) noneatingpos[names[i]][j] = true;
  }
  
  // step 2: analyse all the rules for eating steps, until we can no
  // longer make any new conclusions
  bool changed = true;
  while (changed) {
    changed = false;
    for (i = 0; i < rules.size(); i++) {
      // we work with rules which have all their arguments, to avoid
      // having to make any exceptions
      PTerm l = rules[i]->query_left_side()->copy();
      PTerm r = rules[i]->query_right_side()->copy();
      while (l->query_type()->query_composed()) {
        MetaApplication *ma = new MetaApplication(new Variable(
          l->query_type()->query_child(0)->copy()));
        l = new Application(l, ma);
        r = new Application(r, ma);
      }
    
      // split, get the arguments, and test each argument
      vector<PTerm> parts = l->split();
      // TODO - proper error handling if parts[0] is not a constant -
      // now we'll just crash
      PConstant f = dynamic_cast<PConstant>(parts[0]);
      string fname = f->query_name();

      
      for (int k = 0; k < noneatingpos[fname].size(); k++) {
        PTerm arg = parts[k+1];
        
        // only check positions which haven't already been confirmed
        if (!noneatingpos[fname][k]) continue;
        
        // position k is eating if *any* meta-variable occurring in
        // it might be eaten in r
        Varset MVk = arg->free_var(true);
        
        for (Varset::iterator it = MVk.begin(); it != MVk.end();
                                                it++) {
          int Z = *it;
          if (!at_non_eating_pos(r, Z)) {
            changed = true;
            noneatingpos[fname][k] = false;
            break;
          }
        }
      }
      
      delete l;
      delete r;
    }
  }
}

/**
 * Returns true if the variable Z occurs in s at a position where it
 * cannot disappear (considering the value of noneatingpos); may well
 * return false negatives
 */
bool DependencyGraph :: at_non_eating_pos(PTerm s, int Z) {
  vector<PTerm> splits = s->split();
  PTerm head = splits[0];
  
  if (head->query_meta() || head->query_variable()) {
    PVariable X;
    if (head->query_meta())
      X = dynamic_cast<MetaApplication*>(head)->get_metavar();
    else X = dynamic_cast<PVariable>(head);
    if (X->query_index() == Z) return true;
    else return false;
  }

  if (head->query_abstraction()) {
    if (s->query_abstraction())
      return at_non_eating_pos(s->subterm("1"), Z);
    else // erm, shouldn't happen!
      return false;
  }
  
  if (!head->query_constant()) return false;    // shouldn't happen!
  string headname = head->to_string(false);
  
  for (int i = 0; i < splits.size()-1; i++) {
    if (i >= noneatingpos[headname].size()) {
      if (!is_constructor(head)) break;
    }
    else if (!noneatingpos[headname][i]) continue;
    if (at_non_eating_pos(splits[i+1], Z)) return true;
  }
  
  return false;
}

void DependencyGraph :: get_reduction_info(Alphabet &Sigma) {
  int i;
  
  // step 1: initialise can_reduce_to
  vector<string> names = Sigma.get_all();
  for (i = 0; i < names.size(); i++) {
    for (int j = 0; j < names.size(); j++) {
      can_reduce_to[names[i] + " : " + names[j]] = (i == j);
    }
    can_reduce_to[names[i] + " : #ABS"] = false;
    can_reduce_to[names[i] + " : #VAR"] = false;
  }

  // step 2: note all immediate reductions
  for (i = 0; i < rules.size(); i++) {
    string lhead = rules[i]->query_left_side()->query_head()->to_string(false);
    PTerm right = rules[i]->query_right_side();
    if (right->query_abstraction()) {
      can_reduce_to[lhead + " : #ABS"] = true;
      while (right->query_abstraction()) right = right->subterm("1");
    }
    PTerm rhead = right->query_head();
    if (rhead->query_constant()) {
      can_reduce_to[lhead + " : " + rhead->to_string(false)] = true;
    }
    if (rhead->query_meta() || rhead->query_variable()) {
      // meta-variables might be instantiated with ANYTHING
      for (int j = 0; j < names.size(); j++) {
        can_reduce_to[lhead + " : " + names[j]] = true;
      }
      can_reduce_to[lhead + " : #ABS"] = true;
      can_reduce_to[lhead + " : #VAR"] = true;
    }
  }
  
  // step 3: determine the closure
  bool changed = true;
  while (changed) {
    changed = false;
    for (i = 0; i < names.size(); i++) {
      for (int j = 0; j < names.size(); j++) {
        if (i == j || !can_reduce_to[names[i] + " : " + names[j]]) continue;
        for (int k = -2; k < (int)(names.size()); k++) {
          string name;
          if (k == -2) name = "#VAR";
          else if (k == -1) name = "#ABS";
          else name = names[k];
          if (can_reduce_to[names[j] + " : " + name] &&
              !can_reduce_to[names[i] + " : " + name]) {
            changed = true;
            can_reduce_to[names[i] + " : " + name] = true;
          }
        }
      }
    }
  }
}

/* ===================== calculating cycles ====================== */

void DependencyGraph :: calculate_reachable() {
  int N = graph.size();
  reachable.resize(N);
  for (int i = 0; i < N; i++) {
    // do a floodfill, starting in point i
    vector<int> stack;
    reachable[i].resize(N);
    for (int j = 0; j < N; j++) {
      reachable[i][j] = graph[i][j];
      if (graph[i][j]) stack.push_back(j);
    }
    while (stack.size() > 0) {
      int lastpoint = stack[stack.size()-1];
      stack.pop_back();
      for (int k = 0; k < N; k++) {
        if (graph[lastpoint][k] && !reachable[i][k]) {
          reachable[i][k] = true;
          stack.push_back(k);
        }   
      }   
    }
  }
}

DPSet DependencyGraph :: get_scc() {
  DPSet ret;

  if (reachable.size() != pairs.size()) calculate_reachable();
  int i;
  // for a given point i that is on a cycle, the SCC containing i
  // consists of all dependency pairs which can be reached from i and
  // from which i can be reached
  for (i = 0; i < pairs.size() && !reachable[i][i]; i++);
  if (i == pairs.size()) return ret;
  for (int j = 0; j < pairs.size(); j++) {
    if (reachable[i][j] && reachable[j][i]) ret.push_back(pairs[j]);
  }
  return ret;
}

void DependencyGraph :: remove_pairs(DPSet &dps) {
  vector<int> toremove;
  int i, j;

  for (i = 0; i < dps.size(); i++)
    for (j = 0; j < pairs.size(); j++)
      if (pairs[j] == dps[i]) toremove.push_back(j);

  for (i = 0; i < toremove.size(); i++) {
    int k = toremove[i];
    pairs[k] = NULL;
    for (j = 0; j < pairs.size(); j++) {
      graph[k][j] = false;
      graph[j][k] = false;
    }
  }

  calculate_reachable();
}

vector<DPSet> DependencyGraph :: get_sccs() {
  vector<DPSet> ret;
  vector<int> redo;
  int i, j;

  DPSet scc = get_scc();
  while (scc.size() > 0) {
    // find the indexes of the SCC elements, and set them as
    // not self-reachable (which will make sure they do not get
    // chosen for the next scc), while also storing these numbers
    // in redo (which will make sure they get put back correctly)
    for (i = 0; i < pairs.size(); i++) {
      for (j = 0; j < scc.size(); j++) {
        if (pairs[i] == scc[j]) {
          reachable[i][i] = false;
          redo.push_back(i);
        }
      }
    }
    ret.push_back(scc);
    scc = get_scc();
  }

  // all elements of an scc should be self-reachable!
  for (i = 0; i < redo.size(); i++)
    reachable[redo[i]][redo[i]] = true;

  return ret;
}

/* ==================== output functionality ===================== */

string DependencyGraph :: to_string() {
  string ret = "Dependency Pairs:\n";
  for (int i = 0; i < pairs.size(); i++) {
    if (pairs[i] == NULL) continue;
    char ci[8];
    sprintf(ci, "%d] ", i);
    ret += "  " + string(ci) + pairs[i]->to_string() + "\n";
  }
  ret += "\nConnections:\n";
  for (int j = 0; j < pairs.size(); j++) {
    if (pairs[j] == NULL) continue;
    ret += "  ";
    char cj[6];
    sprintf(cj, "%d", j);
    ret += string(cj) + " --> ";
    for (int k = 0; k < pairs.size(); k++) {
      if (graph[j][k]) {
        char ck[8];
        sprintf(ck, "%d, ", k);
        ret += ck;
      }
    }
    ret += "\n";
  }

  return ret + "\n";
}

void DependencyGraph :: print_self() {
  wout.start_box();
  wout.print_numeric_graph(graph);
  wout.end_box();
}

void DependencyGraph :: print_self(Alphabet &F, ArList &arities) {
  wout.start_box();
  wout.print_DPs(pairs, F, arities, true);
  wout.print_numeric_graph(graph);
  wout.end_box();
}






// ==================================================================
// ==================================================================
// ==================================================================
// ==================================================================
// ==================================================================
// ==================================================================

#ifdef ORIGINALVERSION

DependencyGraph :: DependencyGraph(Alphabet &Sigma, vector<MatchRule*> _rules,
                                   bool enabled)
    :rules(_rules), disabled(!enabled) {
  get_eating_info(Sigma);
  get_reduction_info(Sigma);
}

DependencyGraph :: ~DependencyGraph() {
  for (int i = 0; i < pairs.size(); i++) delete pairs[i];
}

void DependencyGraph :: add_dependency_pair(DependencyPair *pair) {
  // for composed headmost pairs, split in two
  if (pair->query_headmost()) {
    PType type = pair->query_left()->query_type();
    if (type->query_data()) pair->set_headmost(false);
    if (type->query_composed()) {
      PVariable Z = new Variable(type->query_child(0)->copy());
      PTerm MZ = new MetaApplication(Z);
      PTerm l = new Application(pair->query_left()->copy(), MZ);
      PTerm r = new Application(pair->query_right()->copy(), MZ->copy());
      DependencyPair *pair2 = new DependencyPair(l, r, 1);
      add_dependency_pair(pair2);
      pair->set_headmost(false);
    }
  }

  // don't add it if we already know this dependency pair
  int i;
  for (i = 0; i < pairs.size(); i++)
    if (pair->to_string(true) == pairs[i]->to_string(true) &&
        (!pair->query_headmost() || pairs[i]->query_headmost())) return;

  // add the pair
  int pos = pairs.size();
  pairs.push_back(pair);
  graph_entry dummy;
  graph.push_back(dummy);

  // calculate its connections for the graph
  for (i = 0; i < pos; i++) {
    if (connection_possible(pairs[i], pair)) graph[i].push_back(true);
    else graph[i].push_back(false);
  }
  for (i = 0; i < pairs.size(); i++) {
    if (connection_possible(pair, pairs[i])) graph[pos].push_back(true);
    else graph[pos].push_back(false);
  }
}

/* ===================== calculating cycles ====================== */

bool DependencyGraph :: cycle_free() {
  if (pairs.size() == 0) return true;
  if (reachable.size() != pairs.size()) calculate_reachable();

  for (int i = 0; i < pairs.size(); i++) {
    if (reachable[i][i]) return false;
  }
  return true;
}

DPSet DependencyGraph :: get_all_pairs() {
  DPSet ret;
  ret.insert(ret.end(), pairs.begin(), pairs.end());
  return ret;
}

bool DependencyGraph :: prune() {
  if (reachable.size() == 0) calculate_reachable();

  DPSet remove;
  for (int i = 0; i < pairs.size(); i++) {
    if (!reachable[i][i]) remove.push_back(pairs[i]);
  }
  if (remove.size() > 0) remove_pairs(remove);
  return remove.size() > 0;
}

#endif

