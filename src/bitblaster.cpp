/**************************************************************************
   Copyright 2013 Cynthia Kop

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

#include "bitblaster.h"
#include "polconstraintlist.h"
#include "sat.h"
#include <iostream>

int DEBUG = false;

BitBlaster :: BitBlaster(vector<int> &mins, vector<int> &maxs) {
  minima.insert(minima.end(), mins.begin(), mins.end());
  maxima.insert(maxima.end(), maxs.begin(), maxs.end());
  sat_formula = new And();

  TRUEBIT = new_var();
  FALSEBIT = new_var();
  vars.force_value(TRUEBIT, TRUE);
  vars.force_value(FALSEBIT, FALSE);

  // claim variables for all the unknowns, and make them satisfy
  // minimum and maximum
  for (int i = 0; i < minima.size(); i++) {
    // determine the number of bits needed
    if (maxima[i] == -1) maxima[i] = minima[i];
    int max = maxima[i];
    int len = 0;
    while (max > 0) { len++; max /= 2; }
    // if the length is too long, give it the maximum length plus
    // one "overflow" bit
    if (len > MAXBITS) len = MAXBITS + 1;
    numbits.push_back(len);
    // "create" the bits
    int n = vars.query_size();
    vars.add_vars(len);
    bitstart.push_back(n);
    // add requirements for minima and maxima
    inequality_left(i, minima[i]);
    if (len <= MAXBITS) inequality_right(maxima[i], i);
  }
  interestingnums = mins.size();
}

int BitBlaster :: overflow_bound() {
  return 1 << MAXBITS;
}

bool BitBlaster :: solve(PFormula formula, vector<int> &values) {
  SatSolver sat;
  handle_formula(formula);
  delete formula;
  PFormula f = sat_formula->simplify();
  //cout << "f = " << f->to_string() << endl;
  if (!sat.solve(f)) {
    delete f;
    return false;
  }
  f = f->simplify();
  if (!f->query_top()) return false; // shouldn't happen!
  delete f;
  values.clear();
  for (int i = 0; i < interestingnums; i++) {
    values.push_back(recover_number(i));
  }
  return true;
}

void BitBlaster :: inequality(PPol l, PPol r, int conditional) {
  if (l->query_unknown() && r->query_integer()) {
    inequality_left(dynamic_cast<Unknown*>(l)->query_index(),
                    dynamic_cast<Integer*>(r)->query_value(),
                    conditional);
  }
  else if (l->query_integer() && r->query_unknown()) {
    inequality_right(dynamic_cast<Integer*>(l)->query_value(),
                     dynamic_cast<Unknown*>(r)->query_index(),
                     conditional);
  }
  else if (l->query_unknown() && r->query_unknown()) {
    inequality_both(dynamic_cast<Unknown*>(l)->query_index(),
                    dynamic_cast<Unknown*>(r)->query_index(),
                    conditional);
  }
  else {
    cout << "Error: unexpected inequality: " << l->to_string()
         << " >= " << r->to_string() << endl;
  }
}

void BitBlaster :: handle_formula(PFormula formula) {
  if (formula->query_conjunction()) {
    And *a = dynamic_cast<And*>(formula);
    for (int i = 0; i < a->query_number_children(); i++) {
      handle_formula(a->query_child(i));
    }
  }

  else if (formula->query_variable() || formula->query_antivariable()) {
    sat_formula->add_child(formula->copy());
  }

  else if (formula->query_special("integer arithmetic")) {
    IntegerArithmeticConstraint *ia =
      dynamic_cast<IntegerArithmeticConstraint*>(formula);
    inequality(ia->query_left(), ia->query_right());
  }
  
  else if (formula->query_bottom()) {
    sat_formula->add_child(new Bottom);
  }

  else if (formula->query_disjunction()) {
    Or *o = dynamic_cast<Or*>(formula);
    if (o->query_child(0)->query_antivariable() &&
        o->query_child(1)->query_special("integer arithmetic")) {
      AntiVar *x = dynamic_cast<AntiVar*>(o->query_child(0));
      PFormula c = o->query_child(1);
      IntegerArithmeticConstraint *ia =
        dynamic_cast<IntegerArithmeticConstraint*>(c);
      inequality(ia->query_left(), ia->query_right(), x->query_index());
    }

    Or *newo = new Or();
    for (int i = 0; i < o->query_number_children(); i++) {
      PFormula c = o->query_child(i);
      if (c->query_variable() || c->query_antivariable()) {
        newo->add_child(c->copy());
      }
      else if (c->query_special("integer arithmetic")) {
        int v = vars.query_size();
        vars.add_vars(1);
        newo->add_child(new Var(v));
        IntegerArithmeticConstraint *ia =
          dynamic_cast<IntegerArithmeticConstraint*>(c);
        inequality(ia->query_left(), ia->query_right(), v);
      }
      else {
        cout << "Unexpected requirement " << formula->to_string() << endl;
      }
    }
    sat_formula->add_child(newo);
  }
  
  else if (!formula->query_top()) {
    cout << "Unexpected requirement " << formula->to_string() << endl;
  }
}

int BitBlaster :: bit(int index, int i) {
  if (i < 0 || index < 0 || index >= numbits.size() ||
      i >= numbits[index]) cout << "MAYBE\naaaaaargh!" << endl;
  else return bitstart[index] + i;
}

int BitBlaster :: new_var() {
  int ret = vars.query_size();
  vars.add_vars(1);
  return ret;
}

int BitBlaster :: overflow_bit(int index) {
  if (numbits[index] <= MAXBITS)
    cout << "overflow bit queried of number without overflow" << endl;
  else return bit(index, MAXBITS);
}

void BitBlaster :: is_equal(int x, int y, int cond) {
  addreqifnot(new Or(new AntiVar(x), new Var(y)), cond); // x -> y
  addreqifnot(new Or(new Var(x), new AntiVar(y)), cond); // y -> x
}

void BitBlaster :: is_different(int x, int y, int cond) {
  addreqifnot(new Or(new AntiVar(x), new AntiVar(y)), cond); // x -> -y
  addreqifnot(new Or(new Var(x), new Var(y)), cond); // -x -> y
}

void BitBlaster :: is_and(int x, int y, int z, int cond) {
  addreqifnot(new Or(new AntiVar(x), new Var(y)), cond); // x -> y
  addreqifnot(new Or(new AntiVar(x), new Var(z)), cond); // x -> z
  addreqifnot(new Or(new Var(x), new AntiVar(y), new AntiVar(z)), cond);
    // -x -> -y \/ -z
}

void BitBlaster :: is_or(int x, int y, int z, int cond) {
  addreqifnot(new Or(new AntiVar(x), new Var(y), new Var(z)), cond); // x -> y \/ z
  addreqifnot(new Or(new Var(x), new AntiVar(y)), cond); // y -> x
  addreqifnot(new Or(new Var(x), new AntiVar(z)), cond); // z -> x
}

void BitBlaster :: is_atleasttwo(int x, int y, int z, int u, int cond) {
  // if x holds, then at most one of {y,z,u} does not hold
  addreqifnot(new Or(new AntiVar(x), new Var(y), new Var(z)), cond); // x -> y \/ z
  addreqifnot(new Or(new AntiVar(x), new Var(y), new Var(u)), cond); // x -> y \/ u
  addreqifnot(new Or(new AntiVar(x), new Var(z), new Var(u)), cond); // x -> z \/ u
  // if x does not hold, then at most one of {y,z,u} holds
  addreqifnot(new Or(new Var(x), new AntiVar(y), new AntiVar(z)), cond); // -x -> -y \/ -z
  addreqifnot(new Or(new Var(x), new AntiVar(y), new AntiVar(u)), cond); // -x -> -y \/ -u
  addreqifnot(new Or(new Var(x), new AntiVar(z), new AntiVar(u)), cond); // -x -> -z \/ -u
}

void BitBlaster :: is_xor(int x, int y, int z, int cond) {
  // if x holds, then either y /\ -z, or -y /\ z
  addreqifnot(new Or(new AntiVar(x), new Var(y), new Var(z)), cond); // x -> y \/ z
  addreqifnot(new Or(new AntiVar(x), new AntiVar(y), new AntiVar(z)), cond); // x -> -y \/ -z
  // if x does not hold, then y = z
  addreqifnot(new Or(new Var(x), new Var(y), new AntiVar(z)), cond); // -x -> y \/ -z
  addreqifnot(new Or(new Var(x), new AntiVar(y), new Var(z)), cond); // -x -> -y \/ z
}

void BitBlaster :: is_iff(int x, int y, int z, int cond) {
  // if x holds, then either y /\ z or -y /\ -z
  addreqifnot(new Or(new AntiVar(x), new Var(y), new AntiVar(z)), cond); // x -> y \/ -z
  addreqifnot(new Or(new AntiVar(x), new AntiVar(y), new Var(z)), cond); // x -> -y \/ z
  // if x does not hold, then y != z
  addreqifnot(new Or(new Var(x), new Var(y), new Var(z)), cond); // -x -> y \/ z
  addreqifnot(new Or(new Var(x), new AntiVar(y), new AntiVar(z)), cond); // -x -> -y \/ -z
}

PFormula nor(PFormula a, PFormula b, PFormula c, PFormula d) {
  Or *ret = new Or(a, b, c);
  ret->add_child(d);
  return ret;
}

void BitBlaster :: is_triple_xor(int x, int y, int z, int u, int cond) {
  // if x holds, then y or z or u holds;
  addreqifnot(nor(new AntiVar(x), new Var(y), new Var(z), new Var(u)), cond);
  // if x and two of the others hold, then so does the third
  addreqifnot(nor(new AntiVar(x), new AntiVar(y), new AntiVar(z),
                  new Var(u)), cond); // x -> (y /\ z -> u)
  addreqifnot(nor(new AntiVar(x), new AntiVar(y), new Var(z),
                  new AntiVar(u)), cond); // x -> (y /\ u -> z)
  addreqifnot(nor(new AntiVar(x), new Var(y), new AntiVar(z),
                  new AntiVar(u)), cond); // x -> (z /\ u -> y)
  // if x does not hold, then at least one of y, z and u does not hold
  addreqifnot(nor(new Var(x), new AntiVar(y), new AntiVar(z),
                  new AntiVar(u)), cond);
  // if x and two of the others do not hold, then nor does the third
  addreqifnot(nor(new Var(x), new Var(y), new Var(z),
                  new AntiVar(u)), cond); // -x -> (-y /\ -z -> -u)
  addreqifnot(nor(new Var(x), new Var(y), new AntiVar(z),
                  new Var(u)), cond); // -x -> (-y /\ -u -> -z)
  addreqifnot(nor(new Var(x), new AntiVar(y), new Var(z),
                  new Var(u)), cond); // -x -> (-z /\ -u -> -y)
}

int BitBlaster :: recover_number(int index) {
  int ret = 0;
  for (int i = 0; i < numbits[index]; i++) {
    int id = bit(index, i);
    if (vars.query_value(id) == TRUE) ret += 1 << i;
    //else if (vars.query_value(id) != FALSE) return -1;
  }
  return ret;
}

void BitBlaster :: addreq(PFormula formula) {
  if (DEBUG) cout << "addreq(" << formula->to_string() << ")";
  formula = formula->simplify();
  if (DEBUG) cout << "  -- simplifies to " << formula->to_string() << endl;
  if (formula->query_top()) delete formula;
  else sat_formula->add_child(formula);
}

void BitBlaster :: addreqif(PFormula formula, int conditional) {
  if (conditional == -1) addreq(formula);
  else if (formula->query_disjunction()) {
    dynamic_cast<Or*>(formula)->add_child(new AntiVar(conditional));
    addreq(formula);
  }
  else addreq(new Or(new AntiVar(conditional), formula));
}

void BitBlaster :: addreqifnot(PFormula formula, int conditional) {
  if (conditional == -1) addreq(formula);
  else if (formula->query_disjunction()) {
    dynamic_cast<Or*>(formula)->add_child(new Var(conditional));
    addreq(formula);
  }
  else addreq(new Or(new Var(conditional), formula));
}

void BitBlaster :: inequality_left(int unknown, int num, int conditional) {
  // num is 0 ==> always true!
  if (num == 0) return;

  int len = numbits[unknown];
  Or *myor = new Or();
  if (conditional != -1) myor->add_child(new AntiVar(conditional));

  // num is too large for the size of unknown
  if (num >= (1 << len)) { addreq(myor); return; }

  // num is greater than or equal to overflow
  if (len > MAXBITS) {
    int pow2 = 1 << (len-1);
    if (num == pow2) myor->add_child(new Var(overflow_bit(unknown)));
    if (num >= pow2) { addreq(myor); return; }
  }

  int lastbit = len - 1;
  while (lastbit >= 0) {
    for ( ; lastbit >= 0; lastbit--) {
      int pow2 = 1 << lastbit;
      if (pow2 >= num) myor->add_child(new Var(bit(unknown, lastbit)));
      else break;
      if (pow2 == num) { addreq(myor); return; }
    }
    // now, 2^(lastbit+1) > num > 2^lastbit
    Or *mocopy = dynamic_cast<Or*>(myor->copy());
    mocopy->add_child(new Var(bit(unknown, lastbit)));
    addreq(mocopy);
    num -= (1 << lastbit);
    lastbit--;
  }
  // this is not the most efficient method, perhaps, but is quite
  // good when num is typically low, and it avoids introducing extra
  // variables
}

void BitBlaster :: inequality_right(int num, int unknown, int conditional) {
  int len = numbits[unknown];

  // an unknown which occurs on the right-hand side of an inequality
  // may not have an overflow
  if (len > MAXBITS) {
    Or *req = new Or();
    if (conditional != -1) req->add_child(new AntiVar(conditional));
    req->add_child(new AntiVar(overflow_bit(unknown)));
    addreq(req);
    len--;
  }

  Or *req = new Or();
  if (conditional != -1) req->add_child(new AntiVar(conditional));

  // constant in this loop: req \/ num >= unknown_0...unknown_{len-1}
  for (; len > 0; len--) {
    int max = (1 << len) - 1;
    if (num >= max) break;  // num >= unknown_{...} holds!

    int lastbit = len - 1;
    int pow2 = (1 << lastbit);
    if (num < pow2) {
      Or *reqcopy = dynamic_cast<Or*>(req->copy());
      reqcopy->add_child(new AntiVar(bit(unknown, lastbit)));
      addreq(reqcopy);
    }
    else {
      req->add_child(new AntiVar(bit(unknown, lastbit)));
      num -= pow2;
    }
  }

  // req \/ num >= [] must hold, but the latter is already true!
  delete req;
}

void BitBlaster :: inequality_both(int u1, int u2, int conditional) {
  int len1 = numbits[u1];
  int len2 = numbits[u2];

  if (len2 > MAXBITS) {
    addreqif(new AntiVar(overflow_bit(u2)), conditional);
    len2 = MAXBITS;
  }
  while (len2 > len1) {
    int var = bit(u2, len2-1);
    addreqif(new AntiVar(var), conditional);
    len2--;
  }
  Or *req = new Or();
  if (conditional != -1) req->add_child(new AntiVar(conditional));
  while (len1 > len2) {
    len1--;
    req->add_child(new Var(bit(u1,len1)));
  }

  while (len1 > 1) {
    int lastbit = len1 - 1;
    // ABCD >= EFGH if D = 1 /\ H = 0, OR D = H and ABC >= EFG
    // this can also be expressed as: (D = 1 \/ ABC >= EFG) /\
    // (H = 0 \/ ABC >= EFG) /\ (D = 1 \/ H = 0)
    int D = bit(u1, lastbit);
    int H = bit(u2, lastbit);
    int restvar = vars.query_size();
    vars.add_vars(1);
    // D = 1 \/ ABC >= EFG
    Or *reqcopy = dynamic_cast<Or*>(req->copy());
    reqcopy->add_child(new Var(D));
    reqcopy->add_child(new Var(restvar));
    addreq(reqcopy);
    // H = 0 \/ ABC >= EFG
    reqcopy = dynamic_cast<Or*>(req->copy());
    reqcopy->add_child(new AntiVar(H));
    reqcopy->add_child(new Var(restvar));
    addreq(reqcopy);
    // D = 1 \/ H = 0
    req->add_child(new Var(D));
    req->add_child(new AntiVar(H));
    addreq(req);
    // restvar -> ABC >= EFG
    req = new Or(new AntiVar(restvar));
    len1--;
  }

  // len1 = len2 = 1; A >= B if A \/ -B
  req->add_child(new Var(bit(u1,0)));
  req->add_child(new AntiVar(bit(u2,0)));
  addreq(req);
}

vector<int> BitBlaster :: representation(int unknown) {
  vector<int> ret;
  for (int i = 0; i < numbits[unknown]; i++)
    ret.push_back(bit(unknown, i));
  return ret;
}

vector<int> BitBlaster :: number_representation(int num) {
  vector<int> ret;
  if (num >= (1 << MAXBITS)) {
    for (int i = 0; i < MAXBITS; i++)
      ret.push_back(FALSEBIT);
    ret.push_back(TRUEBIT);
  }
  else while (num > 0) {
    if (num % 2 == 1) ret.push_back(TRUEBIT);
    else ret.push_back(FALSEBIT);
    num /= 2;
  }
  return ret;
}

void BitBlaster :: set_squares(map<int,int> &squares) {
  for (map<int,int>::iterator it = squares.begin(); it != squares.end(); it++) {
    //cout << "defining: a" << it->first << " = a" << it->second << " * a" << it->second << endl;
    vector<int> a = representation(it->second);
    vector<int> b = representation(it->second);
    set_product(it->first, a, b);
  }
}

void BitBlaster :: set_unknown_products(PairMap &prods) {
  for (PairMap::iterator it = prods.begin(); it != prods.end(); it++) {
    //cout << "defining: a" << it->first << " = a" << it->second.first << " * a" << it->second.second << endl;
    vector<int> a = representation(it->second.first);
    vector<int> b = representation(it->second.second);
    if (a.size() >= b.size()) set_product(it->first, a, b);
    else set_product(it->first, b, a);
  }
}

void BitBlaster :: set_known_products(PairMap &prods) {
  for (PairMap::iterator it = prods.begin(); it != prods.end(); it++) {
    //cout << "defining: a" << it->first << " = " << it->second.first << " * a" << it->second.second << endl;
    vector<int> a = representation(it->second.second);
    vector<int> b = number_representation(it->second.first);
    set_product(it->first, a, b);
  }
}

void BitBlaster :: set_unknown_sums(PairMap &sums) {
  for (PairMap::iterator it = sums.begin(); it != sums.end(); it++) {
    //cout << "defining: a" << it->first << " = a" << it->second.first << " + a" << it->second.second << endl;
    vector<int> a = representation(it->second.first);
    vector<int> b = representation(it->second.second);
    set_sum(it->first, a, b);
  }
}

void BitBlaster :: set_known_sums(PairMap &sums) {
  for (PairMap::iterator it = sums.begin(); it != sums.end(); it++) {
    //cout << "defining: a" << it->first << " = " << it->second.first << " + a" << it->second.second << endl;
    vector<int> a = representation(it->second.second);
    vector<int> b = number_representation(it->second.first);
    set_sum(it->first, a, b);
  }
}

void BitBlaster :: set_sum(int result, vector<int> &a, vector<int> &b) {
  int alen = a.size(), blen = b.size(), rlen = numbits[result];
  int i;

  // don't count 0-values at the end of either number
  while (alen > 0 && a[alen-1] == FALSEBIT) alen--;
  while (blen > 0 && b[blen-1] == FALSEBIT) blen--;

  // to avoid unnecessary extra cases, assume b has more bits than a
  if (alen > blen) {
    set_sum(result, b, a);
    return;
  }

  if (DEBUG) {
    cout << "result: [";
    for (int i = 0; i < numbits[result]; i++) cout << bit(result, i) << ", ";
    cout << "] length = " << rlen << endl;
    cout << "a: [";
    for (int i = 0; i < a.size(); i++) cout << a[i] << ", ";
    cout << "] length = " << alen << endl;
    cout << "b: [";
    for (int i = 0; i < b.size(); i++) cout << b[i] << ", ";
    cout << "] length = " << blen << endl;
    cout << "TRUEBIT = " << TRUEBIT << ", FALSEBIT = " << FALSEBIT << endl;
  }

  // make sure neither a nor b are too large to fit into result
  for (; alen > rlen; alen--) addreq(new AntiVar(a[alen-1]));
  for (; blen > rlen; blen--) addreq(new AntiVar(b[blen-1]));

  // dismiss those bits of result which will not be used for the sum
  for (; rlen > blen + 1; rlen--) addreq(new AntiVar(bit(result,rlen-1)));

  // if a = 0, special case
  if (alen == 0) {
    if (rlen > blen) addreq(new AntiVar(bit(result, rlen-1)));
    for (i = 0; i < blen; i++) {
      is_equal(bit(result, i), b[i]);
    }
    return;
  }

  // N is the number of bits relevant for the sum
  int N = blen;
  if (N > MAXBITS) N = MAXBITS;

if (DEBUG) cout << "Starting" << endl;

  // determine bits for the carrier, and determine carrier constraints
  // (we do this immediately because they might influence the overflow)
  // note: we try to avoid introducing new variables
  vector<int> carrier;
  for (int i = 0; i < N; i++) {
    // carrier[i] = "at least two of a[i], b[i], carrier[i-1]"
    vector<int> parts;
    parts.push_back(i >= alen ? FALSEBIT : a[i]);
    parts.push_back(b[i]);
    parts.push_back(i == 0 ? FALSEBIT : carrier[i-1]);
    // check which parts we definitely know
    int nfalse = 0, ntrue = 0, unknown;
    for (int j = 0; j < 3; j++) {
      if (parts[j] == TRUEBIT) ntrue++;
      else if (parts[j] == FALSEBIT) nfalse++;
      else unknown = parts[j];
    }
    // if at least two are false or true, we know carrier[i]
    if (nfalse >= 2) carrier.push_back(FALSEBIT);
    else if (ntrue >= 2) carrier.push_back(TRUEBIT);
    else if (nfalse == 1 && ntrue == 1) carrier.push_back(unknown);
    else {
      carrier.push_back(new_var());
      is_atleasttwo(carrier[i], parts[0], parts[1], parts[2]);
    }
  }

if (DEBUG) cout << "got carrier" << endl;

  // the last carrier is special: if rlen == blen, then it must be
  // false, otherwise the last bit of result is determined by the
  // last carrier (and possibly overflow of a and b)
  if (rlen == N) addreq(new AntiVar(carrier[N-1]));
  else if (rlen <= MAXBITS) is_equal(carrier[N-1], bit(result, N));
  else {
    int ovbit = overflow_bit(result);
    if (alen > MAXBITS) addreq(new Or(new AntiVar(a[MAXBITS]),
                                      new Var(ovbit)));
    if (blen > MAXBITS) addreq(new Or(new AntiVar(b[MAXBITS]),
                                      new Var(ovbit)));
    addreq(new Or(new AntiVar(carrier[N-1]), new Var(ovbit)));
    Or *req = new Or(new AntiVar(ovbit));
    if (alen > MAXBITS) req->add_child(new Var(a[MAXBITS]));
    if (blen > MAXBITS) req->add_child(new Var(b[MAXBITS]));
    req->add_child(new Var(carrier[N-1]));
    addreq(req);
  }
if (DEBUG) cout << "handled last carrier" << endl;

  int overflow = -1;
  if (rlen > MAXBITS) overflow = overflow_bit(result);

  // now, add constraints for the other bits of result; if result has
  // overflow, however, we don't need to bother!
  // note that if rlen > N, then the final bit is already dealt with:
  // this bit equals carrier[N-1]
  for (i = 0; i < N; i++) {
    int r = bit(result, i);
    if (i == 0) is_xor(r, a[i], b[i], overflow);
    else if (i >= alen) is_xor(r, b[i], carrier[i-1], overflow);
    else is_triple_xor(r, a[i], b[i], carrier[i-1], overflow);
  }
if (DEBUG) { cout << "end of function" << endl; DEBUG = false; }
}

void BitBlaster :: set_product(int result, vector<int> &a, vector<int> &b) {
  vector< vector<int> > parts;
  int i;

  // we iterate over start, adding new bit-numbers to parts, to
  // eventually sum all of them up!
  // in every step, we make sure that result = <sum of parts> +
  // (a multiplied by 2^start) * (b shifted divided by 2^start)
  for (int start = 0; start < b.size(); start++) {

    // handle 0-bits from a: those just cause shifting in b!
    for (; start < b.size() && b[start] == FALSEBIT; start++);

    // TODO: special case if parts.size() == 0, b[start] is an unknown
    // and b.size() == 1

    // we are going to add newpart to the set of parts
    vector<int> newpart;
    for (i = 0; i < start; i++) newpart.push_back(FALSEBIT);

    // special case: we have reached the overflow of b; now newpart
    // will contain <overflow> if b has overflow and a is non-zero;
    // otherwise, newpart will contain 0
    if (start == MAXBITS) {
      int overflowbit = new_var();
      newpart.push_back(overflowbit);
      // if b has overflow and a is non-zero, then overflowbit must be set
      for (i = 0; i < a.size(); i++)
        addreq(new Or(new AntiVar(b[start]), new AntiVar(a[i]),
               new Var(overflowbit)));
      // overflowbit may only be set under these circumstances
      addreq(new Or(new AntiVar(overflowbit), new Var(b[start])));
      Or *myor = new Or(new AntiVar(overflowbit));
      for (i = 0; i < a.size(); i++)
        myor->add_child(new Var(a[i]));
      addreq(myor);
      // we can skip the rest of the loop
      parts.push_back(newpart);
      continue;
    }

    // after shifting, a should of course still fit in its bounds!
    if (start + a.size() > MAXBITS + 1) {
      int overflowbit = new_var();
      Or *o = new Or(new AntiVar(overflowbit));
      for (i = MAXBITS-start; i < a.size(); i++) {
        o->add_child(new Var(a[i]));
        addreq(new Or(new AntiVar(a[i]), new Var(overflowbit)));
      }
      addreq(o);
      a.resize(MAXBITS+1-start);
      a[MAXBITS-start] = overflowbit;
    }


    // if the currently-first bit of b is known, we simply add the shifted b
    if (b[start] == TRUEBIT)
      for (i = 0; i < a.size(); i++) newpart.push_back(a[i]);

    // and if it is not known, we add either 0 or the shifted b
    else {
      for (i = 0; i < a.size(); i++) {
        int v = new_var();
        newpart.push_back(v);
        // -b[start] -> -v
        addreq(new Or(new Var(b[start]), new AntiVar(v)));
        // b[start] -> v = a[i]
        addreq(new Or(new AntiVar(b[start]), new AntiVar(v), new Var(a[i])));
        addreq(new Or(new AntiVar(b[start]), new Var(v), new AntiVar(a[i])));
      }
    }

    parts.push_back(newpart);
  }

  // sum all parts together!
  if (parts.size() == 0) {
    for (i = 0; i < numbits[result]; i++)
      addreq(new AntiVar(bit(result, i)));
  }

  if (parts.size() == 1) {
    int N = parts[0].size();
    if (numbits[result] < N) N = numbits[result];
    for (i = 0; i < N; i++) is_equal(parts[0][i], bit(result, i));
    for (i = N; i < numbits[result]; i++)
      addreq(new AntiVar(bit(result, i)));
    for (i = N; i < parts[0].size(); i++)
      addreq(new AntiVar(parts[0][i]));
  }

  while (parts.size() >= 2) {
    if (parts.size() == 2) {
      set_sum(result, parts[0], parts[1]);
      break;
    }
    else {
      int num = minima.size();
      minima.push_back(0);
      maxima.push_back((1 << (MAXBITS + 1)));
      bitstart.push_back(vars.query_size());
      numbits.push_back(MAXBITS + 1);
      vars.add_vars(MAXBITS+1);
      set_sum(num, parts[parts.size()-1], parts[parts.size()-2]);
      parts.pop_back();
      parts.pop_back();
      parts.push_back(representation(num));
    }
  }
}

#if 0
/**
 * the overflow bit of c should be set if and only if one of the
 * following holds:
 * - the overflow bit of [a without overflow] * [b without overflow]
 *   is set
 * - a has overflow and b is non-zero
 * - b has overflow and a is non-zero
 */
void BitBlaster :: product_overflow(vector<int> &a, vector<int> &b,
                                    vector<int> &c) {
  int alen = a.size(), blen = b.size(), clen = c.size();
  int i;

  // save some cases by making a the smaller one
  if (alen > blen) {
    product_overflow(b, a, c);
    return;
  }

  // we don't need to do much unless b and c both allow overflow
  // (we assume that blen >= alen)
  if (blen <= MAXBITS) return;
  if (clen <= MAXBITS) {
    addreq(new AntiVar(b[MAXBITS]));
    if (alen > MAXBITS) addreq(new AntiVar(a[MAXBITS]));
    return;
  }

  // replace the overflow bit of c by a new bit, which will be the
  // overflow bit of [a without overflow] * [b without overflow]
  int oldoverflow = c[MAXBITS];
  int newoverflow = new_var();
  c[MAXBITS] = newoverflow;

  // find out whether we know for sure that a is non-zero
  bool anonzero = false, bnonzero = false;
  for (i = 0; i < alen; i++) anonzero |= (a[i] == TRUEBIT);
  for (i = 0; i < blen; i++) bnonzero |= (b[i] == TRUEBIT);

  // if newoverflow is set, so must oldoverflow be
  addreq(new Or(new AntiVar(newoverflow), new Var(oldoverflow)));

  // if b overflows and a is non-zero, oldoverflow must be set
  if (anonzero) addreq(new Or(new AntiVar(b[MAXBITS]), new Var(oldoverflow)));
  else for (i = 0; i < alen; i++) {
    if (a[i] != FALSEBIT)
      addreq(new Or(new AntiVar(b[MAXBITS]),
                     new AntiVar(a[i]), new Var(oldoverflow)));
  }

  // if a overflows and b is non-zero, oldoverflow must be set
  if (alen > MAXBITS) {
    if (bnonzero) addreq(new Or(new AntiVar(a[MAXBITS]),
                                 new Var(oldoverflow)));
    else for (i = 0; i < MAXBITS; i++) {
      if (b[i] != FALSEBIT)
        addreq(new Or(new AntiVar(a[MAXBITS]),
                       new AntiVar(b[i]), new Var(oldoverflow)));
    }
  }

  // of course, if oldoverflow is set, one of the other three
  // situations must hold too!
  
  // if a cannot have overflow, this is exactly given by:
  // oldoverflow -> newoverflow \/ b overflow
  // oldoverflow -> newoverflow \/ a non-zero
  if (alen <= MAXBITS) {
    addreq(new Or(new AntiVar(oldoverflow),
                   new Var(newoverflow),
                   new Var(b[MAXBITS])));
    if (!anonzero) {
      Or *alsononzero = new Or(new AntiVar(oldoverflow),
                               new Var(newoverflow));
      for (i = 0; i < alen; i++)
        alsononzero->add_child(new Var(a[i]));
      addreq(alsononzero);
    }
  }

  // if a can have overflow, we'd better introduce some extra variables
  else {
    int overflowA = bnonzero ? a[MAXBITS] : new_var();
    int overflowB = anonzero ? b[MAXBITS] : new_var();
    addreq(new Or(new AntiVar(oldoverflow),
                   new Var(newoverflow),
                   new Or(new Var(overflowA),
                          new Var(overflowB))));
    if (!bnonzero) {
      addreq(new Or(new AntiVar(overflowA),
                     new Var(a[MAXBITS])));
      Or *myor = new Or(new AntiVar(overflowA));
      for (i = 0; i < blen; i++) myor->add_child(new Var(b[i]));
      addreq(myor);
    }
    if (!anonzero) {
      addreq(new Or(new AntiVar(overflowB),
                     new Var(b[MAXBITS])));
      Or *myor = new Or(new AntiVar(overflowB));
      for (i = 0; i < alen; i++) myor->add_child(new Var(a[i]));
      addreq(myor);
    }
  }
}

void BitBlaster :: create_product(Representation &a, Representation &b,
                                  Representation &result,
                                  And *formula) {
  int last = a[a.size()-1];
  int n = vars.query_size();
  vars.add_vars(b.size());
  Representation z;
  for (int i = 0; i < a.size()-1; i++) z.push_back(0);
  for (int j = 0; j < b.size(); j++) {
    z.push_back(n+j);
    formula->add_child(is_and(n+j, last, b[j]));
  }

  if (a.size() == 1) { result = z; return; }
  Representation y;
  a.pop_back();
  create_product(a, b, y, formula);
  a.push_back(last);
  create_sum(y, z, result, formula);
}

// returns a formula stating that z = a XOR b
PFormula BitBlaster :: is_xor(int z, int a, int b) {
  return nor(nand(new Var(a), new Var(b), new AntiVar(z)),
             nand(new Var(a), new AntiVar(b), new Var(z)),
             nand(new AntiVar(a), new Var(b), new Var(z)),
             nand(new AntiVar(a), new AntiVar(b), new AntiVar(z)));
}

// returns a formula stating that z = a XOR b XOR c
PFormula BitBlaster :: is_xor(int z, int a, int b, int c) {
  return nor(nand(new Var(a), new Var(b), new Var(c), new Var(z)),
             nand(new Var(a), new Var(b), new AntiVar(c), new AntiVar(z)),
             nand(new Var(a), new AntiVar(b), new Var(c), new AntiVar(z)),
             nand(new Var(a), new AntiVar(b), new AntiVar(c), new Var(z)),
             nand(new AntiVar(a), new Var(b), new Var(c), new AntiVar(z)),
             nand(new AntiVar(a), new Var(b), new AntiVar(c), new Var(z)),
             nand(new AntiVar(a), new AntiVar(b), new Var(c), new Var(z)),
             nand(new AntiVar(a), new AntiVar(b), new AntiVar(c), new AntiVar(z)));
}

// returns a formula stating that z = a AND b
PFormula BitBlaster :: is_and(int z, int a, int b) {
  return nor(nand(new Var(a), new Var(b), new Var(z)),
             nand(new AntiVar(z), new AntiVar(a)),
             nand(new AntiVar(z), new AntiVar(b)));
}

PFormula BitBlaster :: nand(PFormula a1, PFormula a2,
                                  PFormula a3, PFormula a4) {
  And *ret = new And();
  if (a1 != NULL) ret->add_child(a1);
  if (a2 != NULL) ret->add_child(a2);
  if (a3 != NULL) ret->add_child(a3);
  if (a4 != NULL) ret->add_child(a4);
  return ret;
}

PFormula BitBlaster :: nor(PFormula a1, PFormula a2,
                                 PFormula a3, PFormula a4,
                                 PFormula a5, PFormula a6,
                                 PFormula a7, PFormula a8) {
  Or *ret = new Or();
  if (a1 != NULL) ret->add_child(a1);
  if (a2 != NULL) ret->add_child(a2);
  if (a3 != NULL) ret->add_child(a3);
  if (a4 != NULL) ret->add_child(a4);
  if (a5 != NULL) ret->add_child(a5);
  if (a6 != NULL) ret->add_child(a6);
  if (a7 != NULL) ret->add_child(a7);
  if (a8 != NULL) ret->add_child(a8);
  return ret;
}

int BitBlaster :: recover_number(Representation &rep) {
  int ret = 0;
  for (int i = 0; i < rep.size(); i++) {
    if (vars.query_value(rep[i]) == TRUE) ret += 1 << i;
    else if (vars.query_value(rep[i]) != FALSE) return -1;
  }
  return ret;
}


/*
bool BitBlaster :: unit_propagate(And *formula) {
  bool donestuff = false, changed = true;
  while (changed) {
    changed = false;
    for (int i = 0; i < formula->query_number_children(); i++) {
      PFormula child = formula->query_child(i)->simplify();
      if (child->query_variable() || child->query_antivariable()) {
        changed = donestuff = true;
        dynamic_cast<Atom*>(child)->force();
        child = new Top();
      }
      if (child != formula->query_child(i))
        formula->replace_child(i, child);
    }
  }
  return donestuff;
}

bool BitBlaster :: forced_zero(And *formula) {
  set<int> mustbezero;
  bool anything_changed = false;

  for (int i = 0; i < formula->query_number_children(); i++) {
    PFormula current = formula->query_child(i)->simplify();
    if (!current->query_special("integer arithmetic")) continue;
    IntegerArithmeticConstraint* cur =
      dynamic_cast<IntegerArithmeticConstraint*>(current);
    PPol left = cur->query_left();
    PPol right = cur->query_right();

    // we'll redo some of the "trivial checks" from the
    // PolConstraintList, since we may get simpler constraints after
    // forcing stuff

    // if both sides are integers, we know how they compare
    if (left->query_integer() && right->query_integer()) {
      int a = dynamic_cast<Integer*>(left)->query_value();
      int b = dynamic_cast<Integer*>(right)->query_value();
      if (a >= b) {
        delete formula->replace_child(i, new Top());
        anythingchanged = true;
        continue;
      }
      else {
        delete formula->replace_child(i, new Bottom());
        return true;
      }
    }
    
    // we can do a lot if left is 0
    if (left->query_integer() &&
        dynamic_cast<Integer*>(left)->query_value() == 0) {

      anything_changed = true;
      
      // 0 >= a + b iff 0 >= a and 0 >= b
      if (right->query_sum()) {
        Sum *r = dynamic_cast<Sum*>(right);
        for (int j = 0; j < r->number_children(); j++) {
          PPol a = new Integer(0);
          PPol b = r->get_child(j)->copy();
          formula->add_child(new IntegerArithmeticConstraint(a, b));
        }
        delete formula->replace_child(i, new Top());
      }

      // 0 >= a * b iff 0 >= a or 0 >= b
      else if (right->query_product()) {
        Product *r = dynamic_cast<Product*>(right);
        Or *newform = new Or();
        for (int j = 0; j < r->number_children(); j++) {
          PPol a = new Integer(0);
          PPol b = r->get_child(j)->copy();
          newform->add_child(new IntegerArithmeticConstraint(a, b));
        }
        delete formula->replace_child(i, newform);
      }
      
      // otherwise we must have an unknown - which must be 0
      else {
        mustbezero.insert(dynamic_cast<Unknown*>(right)->query_index());
        delete formula->replace_child(i, new Top());
      }
    }

    // we can also often do stuff if right is 1
    if (right->query_integer() &&
        dynamic_cast<Integer*>(right)->query_value() == 1) {
      
      anything_changed = true;
    
      // a + b >= 1 iff a >= 1 or b >= 1
      if (left->query_sum()) {
        Sum *l = dynamic_cast<Sum*>(left);
        Or *newform = new Or();
        for (int j = 0; j < l->number_children(); j++) {
          PPol a = l->get_child(i)->copy();
          PPol b = new Integer(1);
          newform->add_child(new IntegerArithmeticConstraint(a, b));
        }
        delete formula->replace_child(i, newform);
      }

      // a * b >= 1 iff a >= 1 and b >= 1
      else if (left->query_product()) {
        Product *l = dynamic_cast<Product*>(left);
        for (int j = 0; j < l->number_children(); j++) {
          PPol a = l->get_child(i)->copy();
          PPol b = new Integer(1);
          formula->add_child(new IntegerArithmeticConstraint(a, b));
        }
        delete formula->replace_child(i, new Top());
      }

      // if a is an unknown, well, just mark that it must be 1!
    }    
  
        // a * b >= 1 iff a >= 1 and b >= 1
        if (left->query_product()) {
          Product *l = dynamic_cast<Product*>(left);
          for (int i = 0; i < l->number_children(); i++) {
            int nc = var_for_constraint(l->get_child(i)->copy(), new Integer(1));
            add_formula(new Or(new AntiVar(vindex), new Var(nc)));
          }
          return true;
        }
    
        // x >= 1 and F(x) >= 1 do not in general hold
        if (left->query_variable() || left->query_functional()) {
          add_formula(new AntiVar(vindex));
          return true;
        }
      }
    }
  }

  return anything_changed;
}

PFormula BitBlaster :: simplify_conjunction(And *f) {
  PFormula formula;
  bool ready = false;

  while (!ready) {
    ready = true;

    if (unit_propagate(f)) {
      formula = f->simplify();
      if (!formula->query_conjunction()) return formula;
      f = dynamic_cast<And*>(formula);
    }
  
    if (forced_zero(f)) {
      formula = f->simplify();
      if (!formula->query_conjunction()) return formula;
      f = dynamic_cast<And*>(formula);
      ready = false;
    }
  }
}
  */
#endif

