/**************************************************************************
   Copyright 2024 Cynthia Kop

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

#include "inputreaderari.h"
#include "outputmodule.h"
#include "type.h"
#include <fstream>
#include <iostream>

int InputReaderARI :: read_file(string filename, Alphabet &Sigma,
                                vector<MatchRule*> &rules) {

  ifstream file(filename.c_str());
  
  string txt;
  while (!file.eof()) {
    string input;
    getline(file, input);
    txt += input + "\n";
  }

  return read_text(txt, Sigma, rules);
}

string to_string_position(int row, int col) {
  return to_string(row) + ":" + to_string(col);
}

string to_string_position(Token token) {
  return to_string_position(token.row, token.col);
}

string to_string(Token token) {
  return to_string_position(token.row, token.col) + ": " + token.text;
}

int InputReaderARI :: read_text(string txt, Alphabet &Sigma,
                                vector<MatchRule*> &rules) {
  vector<Token> tokens;
  if (!tokenise(txt, tokens)) return 1;
  for (int i = 0; i < tokens.size(); i++) {
    if (tokens[i].kind == ID) tokens[i].text = fix_name(tokens[i].text);
  }
  ParseTree *tree = make_tree(tokens);
  if (tree == NULL) return 1;
  if (!verify_shape(tree)) return 1;

  set<string> sorts;
  int ret = 0;
  for (int i = 0; i < tree->subtrees.size(); i++) {
    ParseTree *component = tree->subtrees[i];
    if (component->subtrees[0]->token->kind == FORMAT) {
      ret = read_format(component);
    }
    else if (component->subtrees[0]->token->kind == SORT) {
      ret = read_sort(component, sorts);
    }
    else if (component->subtrees[0]->token->kind == FUN) {
      ret = read_func(component, sorts, Sigma);
    }
    else if (component->subtrees[0]->token->kind == RULE) {
      ret = read_rule(component, sorts, Sigma, rules);
    }
    if (ret != 0) return ret;
  }
  delete tree;
  return 0;
}

string InputReaderARI :: query_warning() {
  string ret = last_warning;
  last_warning = "";
  return ret;
}

/** Helper function for tokenise */
bool identifier_letter(char c) {
  return (c >= '0' && c <= '9') ||
         (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         c == '~' || c == '!' || c == '@' || c == '$' || c == '%' ||
         c == '^' || c == '&' || c == '*' || c == '_' || c == '-' ||
         c == '+' || c == '=' || c == '<' || c == '>' || c == '.' ||
         c == '?' || c == '/';
}

bool InputReaderARI :: tokenise(string txt, vector<Token> &tokens) {
  int i = 0, row = 1, col = 1;
  while (i < txt.length()) {
    // read past whitespace
    if (txt[i] == '\n') { row++; col = 1; i++; continue; }
    if (txt[i] == ' ' || txt[i] == '\t') { col++; i++; continue; }
    // read past comments
    if (txt[i] == ';') {
      while (i < txt.length() && txt[i] != '\n') i++;
      continue;
    }
    // read past a quoted symbol
    if (txt[i] == '|') {
      int endcol = col, endrow = row, j;
      for (j = i + 1; j < txt.length() && txt[j] != '|'; j++) {
        if (txt[j] == '\n') { endrow++; endcol = 0; }
        else endcol++;
        if (txt[j] == '\\') {
          last_warning = "Illegal escape in quote at " +
            to_string_position(endrow, endcol) + ".";
          return false;
        }
      }
      if (j == txt.length()) {
        last_warning = "Unclosed quote at " + to_string_position(row, col)
          + ".";
        return false;
      }
      tokens.push_back({ row, col, ID, txt.substr(i+1, j-i-1) });
      i = j+1;
      row = endrow;
      col = endcol+2;
      continue;
    }

    TokenKinds kind = ID;
    int len = 1;
    if (txt[i] == '(') { kind = BROPEN; len = 1; }
    else if (txt[i] == ')') { kind = BRCLOSE; len = 1; }
    else if (identifier_letter(txt[i])) {
      len = 1;
      while (i + len < txt.length() && identifier_letter(txt[i+len])) len++;
      string text = txt.substr(i, len);
      if (text == "format") kind = FORMAT;
      else if (text == "fun") kind = FUN;
      else if (text == "sort") kind = SORT;
      else if (text == "rule") kind = RULE;
      else if (text == "lambda") kind = LAMBDA;
      else if (text == "->") kind = ARROW;
      else kind = ID;
    }
    else {
      last_warning = "Illegal character at " + to_string_position(row, col)
        + ": " + txt.substr(i, 1);
      return false;
    }

    tokens.push_back({ row, col, kind, txt.substr(i, len) });
    col += len;
    i += len;
  }
  return true;
}

/**
 * Yes, there is code copying here from XMLReader. Wanda needs some solid
 * refactoring to have assumptions like the naming standard in one place
 * (or even better, Wanda really shouldn't be relying on a naming standard
 * at all!).  As it is, this would require a major overhaul, so I'm
 * choosing to simply copy.  I will do the pushups. :)
 */
string InputReaderARI :: fix_name(string name) {
  for (int i = 0; i < name.length(); i++) {
    char c = name[i];
    if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
        ('0' <= c && c <= '9')) continue;
    string rep;
    if (c == '!') rep = "!fac";
    else if (c == '.') rep = "!dot";
    else if (c == '*') rep = "!times";
    else if (c == '+') rep = "!plus";
    else if (c == '-') rep = "!minus";
    else if (c == '/') rep = "!div";
    else if (c == '$') rep = "!dollar";
    else if (c == ' ') rep = "!space";
    else if (c == '\n') rep = "!newline";
    else {
      char ss[20];
      ss[0] = '!';
      int j = 1;
      while (c != 0) {
        ss[j] = (unsigned char)(c) % 9 + '1';
        c = c / 9;
        j++;
      }
      ss[j] = '0';
      ss[j+1] = '\0';
      rep = string(ss);
    }
    name = name.substr(0,i) + rep + name.substr(i+1);
  }
  return name;
}

/**
 * This builds a parse tree from the given token list.
 * WARNING: all the token pointers in this tree point to elements of the tokens
 * vector.  So, if this vector goes out of scope, the parse tree should be
 * deleted as well.
 */
ParseTree *InputReaderARI :: make_tree(vector<Token> &tokens) {
  vector<ParseTree*> status;
  status.push_back(new ParseTree());
  for (int i = 0; i < tokens.size(); i++) {
    if (tokens[i].kind == BROPEN) status.push_back(new ParseTree());
    else if (tokens[i].kind == BRCLOSE) {
      ParseTree *result = status.back();
      status.pop_back();
      if (status.size() == 0) {
        last_warning = "Unexpected closing bracket at " +
          to_string_position(tokens[i]);
        return NULL;
      }
      status.back()->subtrees.push_back(result);
    }
    else status.back()->subtrees.push_back(new ParseTree(&tokens[i]));
  }
  if (status.size() != 1) {
    last_warning = "not all opening brackets are closed";
    return NULL;
  }
  return status[0];
}

bool InputReaderARI :: verify_shape(ParseTree *tree) {
  if (tree->token != NULL) {
    last_warning = "unexpected token: " + to_string(*tree->token);
    return false;
  }
  for (int i = 0; i < tree->subtrees.size(); i++) {
    ParseTree *component = tree->subtrees[i];
    /* component should be a format declaration, sort declaration, function
     * declaration or rule */
    if (component->token != NULL) {
      last_warning = "unexpected token: " + to_string(*component->token);
      return false;
    }
    if (component->subtrees.size() == 0) {
      last_warning = "unexpected empty subtree ()";
      return false;
    }
    if (component->subtrees[0]->token == NULL) {
      last_warning = "unexpected subtree " + to_string(i);
      return false;
    }
    if (component->subtrees[0]->token->kind != FORMAT &&
        component->subtrees[0]->token->kind != SORT &&
        component->subtrees[0]->token->kind != FUN &&
        component->subtrees[0]->token->kind != RULE) {
      last_warning = "unexpected token: " +
        to_string(*component->subtrees[0]->token) +
        " (expected format, sort, fun or rule)";
      return false;
    }
  }
  return true;
}

/**
 * This checks that the format line has the right shape, and either returns
 * 0 (if it does), 1 (if it doesn't), or 2 (if it kinda has the right shape,
 * but it's the wrong format).
 */
int InputReaderARI :: read_format(ParseTree *tree) {
  if (tree->subtrees.size() != 2 ||
      tree->subtrees[1]->token == NULL ||
      tree->subtrees[1]->token->kind != ID) {
    last_warning = "Unexpected format line: expected format <format>";
    return 1;
  }
  if (tree->subtrees[1]->token->text != "higher-order" &&
      tree->subtrees[1]->token->text != "higher!minusorder") {
    wout.debug_print("Format is not higher-order!");
    return 2; // MAYBE: we can't handle this format!
  }
  return 0;
}

/**
 * Given that tokens[start..end] represents a sort line (not including the
 * opening and closing bracket), this adds the declared sort into sorts.
 */
int InputReaderARI :: read_sort(ParseTree *tree, set<string> &sorts) {
  if (tree->subtrees.size() != 2 ||
      tree->subtrees[1]->token == NULL ||
      tree->subtrees[1]->token->kind != ID) {
    last_warning = "Unexpected sort line: expected sort <identifier>";
    return 1;
  }
  string name = tree->subtrees[1]->token->text;
  if (sorts.find(name) != sorts.end()) {
    last_warning = to_string_position(*tree->subtrees[1]->token) +
      ": Duplicate sort declaration for " + name + ".";
    return 1;
  }
  sorts.insert(name);
  return 0;
}

int InputReaderARI :: read_func(ParseTree *tree, set<string> &sorts,
                                Alphabet &Sigma) {
  if (tree->subtrees.size() != 3 ||
      tree->subtrees[1]->token == NULL ||
      tree->subtrees[1]->token->kind != ID) {
    last_warning = "Unexpected fun line at " +
      to_string_position(*tree->subtrees[0]->token) +
      ": expected fun <identifier> <type>";
    return 1;
  }
  string name = tree->subtrees[1]->token->text;
  if (Sigma.get(name) != NULL) {
    last_warning = "Double declaration of function symbol " + name + ".";
    return 1;
  }
  PType type = read_type(tree->subtrees[2], sorts);
  if (type == NULL) return 1;
  Sigma.add(name,type);
  return 0;
}

PType InputReaderARI :: read_type(ParseTree *tree, set<string> &sorts) {
  // it's an identifier!
  if (tree->token != NULL) {
    if (tree->token->kind != ID) {
      last_warning = "Illegal token at " + to_string_position(*tree->token) +
        "; expected a type";
      return NULL;
    }
    string name = tree->token->text;
    if (sorts.find(name) != sorts.end()) return new DataType(name);
    last_warning = "Undeclared sort " + name + " at " +
      to_string_position(*tree->token) + ".";
    return NULL;
  }
  // it's a list!
  if (tree->subtrees.size() < 3 ||
      tree->subtrees[0]->token == NULL ||
      tree->subtrees[0]->token->kind != ARROW) {
    last_warning = "Encountered type with an unexpected shape.";
    return NULL;
  }
  int last = tree->subtrees.size()-1;
  if (tree->subtrees[last]->token == NULL) {
    last_warning = "Encountered type with an unexpected shape at " +
      to_string_position(*tree->subtrees[0]->token);
    return NULL;
  }
  PType output = read_type(tree->subtrees[last], sorts);
  if (output == NULL) return NULL;
  for (last = last - 1; last > 0; last--) {
    PType inp = read_type(tree->subtrees[last], sorts);
    if (inp == NULL) { delete output; return NULL; }
    else output = new ComposedType(inp, output);
  }
  return output;
}

int InputReaderARI :: read_rule(ParseTree *tree, set<string> &sorts,
                                Alphabet &Sigma, vector<MatchRule*> &rules) {
  if (tree->subtrees.size() != 3) {
    last_warning = "Unexpected rule line at " +
      to_string_position(*tree->subtrees[0]->token) +
      ": expected rule <term> <term>";
    return 1;
  }
  map<string,PVariable> variables;
  map<string,int> meta_arities;
  PTerm left = read_term(tree->subtrees[1], sorts, Sigma, variables,
                         meta_arities, NULL, true);
  if (left == NULL) return 1;
  PTerm right = read_term(tree->subtrees[2], sorts, Sigma, variables,
                          meta_arities, left->query_type(), false);
  if (right == NULL) return 1;
  MatchRule *rule = new MatchRule(left, right);
  for (map<string,PVariable>::iterator it = variables.begin();
       it != variables.end(); it++) delete it->second;
  rules.push_back(rule);
  return 0;
}

PTerm InputReaderARI :: read_term(ParseTree *tree, set<string> &sorts,
                                  Alphabet &Sigma,
                                  map<string,PVariable> &variables,
                                  map<string,int> &arities,
                                  PType expected, bool lhs) {
  if (tree->token != NULL) {
    return read_single_token_term(tree->token, sorts, Sigma, variables,
                                  arities, expected, lhs);
  }

  if (tree->subtrees.size() == 0 ||
      tree->subtrees[0]->token == NULL) {
    last_warning = "unexpected term tree; does not start with identifier "
      "or lambda";
    return NULL;
  }

  if (tree->subtrees[0]->token->kind == ID) {
    return read_application(tree, sorts, Sigma, variables, arities,
                            expected, lhs);
  }

  if (tree->subtrees[0]->token->kind == LAMBDA) {
    return read_abstraction(tree, sorts, Sigma, variables, arities,
                            expected, lhs);
  }

  last_warning = "unexpected token at position " +
    to_string_position(*tree->subtrees[0]->token) + "; expected "
    "identifier or lambda";
  return NULL;
}

PTerm InputReaderARI :: verify_type(PTerm term, PType expected) {
  if (expected == NULL) return term;
  if (term->query_type()->equals(expected)) return term;
  last_warning = "Term " + term->to_string() + " has unexpected type " +
    term->query_type()->to_string() + "; expected " +
    expected->to_string() + ".";
  delete term;
  return NULL;
}

PTerm InputReaderARI :: read_single_token_term(Token *token,
                          set<string> &sorts, Alphabet &Sigma,
                          map<string,PVariable> &variables,
                          map<string,int> &arities,
                          PType expected, bool lhs) {
  if (token->kind != ID) {
    last_warning = "unexpected token: " + to_string(*token) +
      ": expected IDENTIFIER";
    return NULL;
  }
  string name = token->text;
  // is it a constant?
  PTerm ret = Sigma.get(name);
  if (ret != NULL) return verify_type(ret, expected);
  // is it a known variable?
  map<string,PVariable>::iterator it = variables.find(name);
  if (it != variables.end()) {
    if (arities[name] > 0) {
      last_warning = "inconsistent use of variable " + name + " at position " +
        to_string_position(*token) + ": previously used with arity " +
        to_string(arities[name]);
      return NULL;
    }
    PVariable x = new Variable(it->second);
    if (arities[name] < 0) return verify_type(x, expected);
    else return verify_type(new MetaApplication(x), expected);
  }
  // it's an unknown variable -- are we allowed to do that?
  if (!lhs) {
    last_warning = "unexpected token " + to_string(*token) + ": fresh " +
      "variables are not allowed in the right-hand side of a rule";
    return NULL;
  }
  if (expected == NULL) {
    last_warning = "Could not derive type of variable " + name +
      " at " + to_string_position(*token) + ".";
    return NULL;
  }
  // it's an unknown variable and we are allowed to do that!
  PVariable x = new Variable(expected->copy());
  variables[name] = x;
  arities[name] = 0;
  return new MetaApplication(new Variable(x));
}

PTerm InputReaderARI :: read_application(ParseTree *tree,
                          set<string> &sorts, Alphabet &Sigma,
                          map<string,PVariable> &variables,
                          map<string,int> &arities,
                          PType expected, bool lhs) {
  string name = tree->subtrees[0]->token->text;
  // if it's a constant, we need to parse it as an application: read the
  // arguments, and then just apply the head to it
  PTerm head = Sigma.get(name);
  if (head != NULL) {
    return read_application(head, tree, sorts, Sigma,
                            variables, arities, expected, lhs);
  }
  // if it's a known meta-variable of arity 0 or binder variable, and we're
  // in the right-hand side, we also parse it as an application
  map<string,PVariable>::iterator it = variables.find(name);
  PVariable x = NULL;
  if (it != variables.end()) {
    x = new Variable(it->second);
    if (!lhs) {
      if (arities[name] < 0) {
        return read_application(x, tree, sorts, Sigma, variables,
                                arities, expected, lhs);
      }
      else if (arities[name] == 0) {
        return read_application(new MetaApplication(x), tree, sorts, Sigma,
                                variables, arities, expected, lhs);
      }
    }
  }
  // if it's a known variable of arity > 0, but we don't have that number of
  // arguments here, throw up an error
  if (x != NULL && arities[name] != tree->subtrees.size() - 1) {
    last_warning = "Inconsistent use of arity: " + name + " was previously "
      "used with arity " +
      (arities[name] > 0 ? to_string(arities[name]) : "0") +
      " but is used with " + to_string(tree->subtrees.size() - 1) +
      " arguments at " + to_string_position(*tree->subtrees[0]->token) + ".";
    delete x;
    return NULL;
  }
  // if we're in the right-hand side, the variable must be known, and since
  // the arity matches previous uses we must end up with a meta-application
  if (!lhs) {
    if (x == NULL) {
      last_warning = "unexpected token " +
        to_string(*tree->subtrees[0]->token) + ": fresh variables " +
        "are not allowed in the right-hand side of a rule";
      return NULL;
    }
    else return read_meta_application(x, tree, sorts, Sigma, variables,
                                      arities, expected, lhs);
  }
  // remaining case: we're in the left-hand side, so this must be a pattern
  if (!check_all_bound_variables(tree, variables, arities)) {
    if (x != NULL) delete x;
    return NULL;
  }
  vector<PTerm> args;
  for (int i = 1; i < tree->subtrees.size(); i++) {
    args.push_back(new Variable(variables[tree->subtrees[i]->token->text]));
  }
  // whether the variable is something we have already seen or not, we should
  // have an expectation for it
  if (expected == NULL) {
    last_warning = "Meta-variable occurrence in left-hand side, " +
      to_string(*tree->subtrees[0]->token) + ", not allowed at a position " +
      "where no type expectation is given.";
    return NULL;
  }
  PType ftype = expected->copy();
  for (int i = args.size()-1; i >= 0; i--) {
    ftype = new ComposedType(args[i]->query_type()->copy(), ftype);
  }
  // x = NULL => it's the first occurrence of a pattern for this meta-variable;
  // create it
  if (x == NULL) {
    x = new Variable(ftype);
    variables[name] = new Variable(x);
    arities[name] = args.size();
  }
  // x != NULL => it should have the right type!
  else {
    if (ftype->equals(x->query_type())) delete ftype;
    else {
      last_warning = "Meta-variable has inconsistent type: " +
        to_string(*tree->subtrees[0]->token);
      delete ftype;
      delete x;
    }
  }
  return new MetaApplication(x, args);
}

/**
 * Given a tree of the form F(s1,...,sn), this checks that all si are bound
 * variables according to variables and arities.  No memory is allocated or
 * freed.
 */
bool InputReaderARI :: check_all_bound_variables(ParseTree *tree,
                                            map<string,PVariable> &variables,
                                            map<string,int> &arities) {
  for (int i = 1; i < tree->subtrees.size(); i++) {
    if (tree->subtrees[i]->token == NULL) {
      last_warning = "[" + to_string(*tree->subtrees[0]->token) + "]: "
        "variable in the left-hand side should only be applied to bound "
        "variables, not complex terms";
      return false;
    }
    if (tree->subtrees[i]->token->kind != ID) {
      last_warning = "unexpected token " + to_string(*tree->subtrees[i]->token)
        + ": expected a bound variable";
      return false;
    }
    string name = tree->subtrees[i]->token->text;
    if (variables.find(name) == variables.end()) {
      last_warning = "Unexpected token " + to_string(*tree->subtrees[i]->token)
        + ": expected a bound variable, not a previously unseen identifier.";
      return false;
    }
    if (arities[name] != -1) {
      last_warning = "Unexpected token " + to_string(*tree->subtrees[i]->token)
        + ": " + name + " was previously used as a meta-variable, not a " +
        "bound variable";
      return false;
    }
  }
  return true;
}

/**
 * Helper function for the main read_application method: given that the given
 * subtree *actually* represents an application -- and not a meta-variable --
 * headed by the given head, this reads it into a term.
 * If reading fails, then head is deleted and NULL returned.
 */
PTerm InputReaderARI :: read_application(PTerm head, ParseTree *tree,
                                set<string> &sorts, Alphabet &Sigma,
                                map<string,PVariable> &variables,
                                map<string,int> &arities,
                                PType expected, bool lhs) {
  for (int i = 1; i < tree->subtrees.size(); i++) {
    if (!head->query_type()->query_composed()) {
      last_warning = "Type error: " + (tree->subtrees[0]->token->text) +
        " used with too many arguments at " +
        to_string_position(*tree->subtrees[0]->token) + ".";
      delete head;
      return NULL;
    }
    PType inp = head->query_type()->query_child(0);
    PTerm arg = read_term(tree->subtrees[i], sorts, Sigma, variables,
                          arities, inp, lhs);
    if (arg == NULL) { delete head; return NULL; }
    head = new Application(head, arg);
  }
  return verify_type(head, expected);
}

/**
 * Helper function for the main read_application method: given that the given
 * subtree represents a meta-application with F as the meta-variable, and
 * given that F is already stored in variables and arities and the arity is
 * correct, this reads the full term and returns it.
 * If reading fails, then F is deleted and NULL returned.
 */
PTerm InputReaderARI :: read_meta_application(PVariable F, ParseTree *tree,
                                set<string> &sorts, Alphabet &Sigma,
                                map<string,PVariable> &variables,
                                map<string,int> &arities,
                                PType expected, bool lhs) {
  PType type = F->query_type();
  vector<PTerm> args;
  bool errored = false;
  for (int i = 1; i < tree->subtrees.size() && !errored; i++) {
    PType inp = type->query_child(0);
    type = type->query_child(1);
    PTerm arg = read_term(tree->subtrees[i], sorts, Sigma, variables,
                          arities, inp, lhs);
    if (arg == NULL) errored = true;
    else args.push_back(arg);
  }
  if (expected != NULL && !type->equals(expected)) {
    last_warning = "Type error: output type of " +
      to_string(*tree->subtrees[0]->token) + " is " +
      type->to_string() + " while I expected a type " +
      expected->to_string();
    errored = true;
  }

  if (!errored) return new MetaApplication(F, args);
  delete F;
  for (int i = 0; i < args.size(); i++) delete args[i];
  return NULL;
}

PTerm InputReaderARI :: read_abstraction(ParseTree *tree,
                          set<string> &sorts, Alphabet &Sigma,
                          map<string,PVariable> &variables,
                          map<string,int> &arities,
                          PType expected, bool lhs) {
  if (tree->subtrees.size() != 3 ||
      tree->subtrees[1]->token != NULL) {
    last_warning = "Unexpected subterm at " +
      to_string_position(*tree->subtrees[0]->token) +
      ": expected lambda vars subterm";
    return NULL;
  }

  ParseTree *vartree = tree->subtrees[1];
  vector<string> names;
  vector<PVariable> abstracted;
  bool errored = false;
  for (int i = 0; i < vartree->subtrees.size(); i++) {
    ParseTree *vardec = vartree->subtrees[i];
    if (vardec->token != NULL || vardec->subtrees.size() != 2 ||
        vardec->subtrees[0]->token == NULL ||
        vardec->subtrees[0]->token->kind != ID) {
      last_warning = "Variable " + to_string(i + 1) + " of abstraction at " +
        to_string_position(*tree->subtrees[0]->token) + " has an unexpected "
        "shape: expected (IDENTIFIER <type>).";
      return NULL;
    }
    if (expected != NULL && !expected->query_composed()) {
      last_warning = "Variable " + to_string(*vardec->subtrees[0]->token) +
        " cannot be used: the type of this abstraction does not have so "
        "many input types.";
      return NULL;
    }
    string name = vardec->subtrees[0]->token->text;
    if (variables.find(name) != variables.end()) {
      last_warning = "Ambiguous use of binder variable " + name + " at " +
        to_string_position(*vardec->subtrees[0]->token);
      return NULL;
    }
    PType type = read_type(vardec->subtrees[1], sorts);
    if (type == NULL) { errored = true; break; }
    if (expected != NULL) {
      if (!type->equals(expected->query_child(0))) {
        last_warning = "Variable " + to_string(*vardec->subtrees[0]->token) +
          " has type " + type->to_string() + " while I expected " +
          expected->query_child(0)->to_string();
        return NULL;
      }
      expected = expected->query_child(1);
    }
    names.push_back(name);
    abstracted.push_back(new Variable(type));
  }

  PTerm term = NULL;
  if (!errored) {
    for (int i = 0; i < names.size(); i++) {
      variables[names[i]] = abstracted[i];
      arities[names[i]] = -1;
    }
    term = read_term(tree->subtrees[2], sorts, Sigma, variables,
                     arities, expected, lhs);
    for (int i = 0; i < names.size(); i++) {
      variables.erase(variables.find(names[i]));
      arities.erase(arities.find(names[i]));
    }
  }
  errored |= (term == NULL);

  if (!errored) {
    for (int i = abstracted.size() - 1; i >= 0; i--) {
      term = new Abstraction(abstracted[i], term);
    }
    return term;
  }

  for (int i = 0; i < abstracted.size(); i++) delete abstracted[i];
  return NULL;
}

