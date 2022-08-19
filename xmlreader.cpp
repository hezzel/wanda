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

#include "xmlreader.h"
#include <vector>
#include <fstream>

string XMLReader :: read_file(string filename) {
  string strategy;
  return read_file(filename, strategy);
}

string XMLReader :: read_file(string filename, string &strategy) {
  string alphabet;
  string variables;
  string rules;

  transform_html = false;

  // read the file
  ifstream file(filename.c_str());
  string txt;
  if (!file.fail()) {
    while (!file.eof()) {
      string input;
      getline(file, input);
      txt += input;
    }
  }

  if (txt.find("<functionSymbolTypeInfo>") == string::npos) return "TRS";

  // remove any whitespace
  int whitespace = txt.find(" ");
  while (whitespace != string::npos) {
    txt.erase(whitespace, 1); 
    whitespace = txt.find(" ", whitespace);
  }
  whitespace = txt.find(string(1,9));
  while (whitespace != string::npos) {
    txt.erase(whitespace, 1); 
    whitespace = txt.find(string(1,9), whitespace);
  }

  // check whether there's a strategy
  int end;
  strategy = get_part(txt, "strategy", 0, end);
  if (strategy == "") strategy = "FULL";

  // get the individual parts
  string svar = get_first_part(txt, "variableTypeInfo");
  string sfun = get_first_part(txt, "functionSymbolTypeInfo");
  string srul = get_first_part(txt, "rules");

  // parse the alphabet
  while (sfun.substr(0,17) == "<funcDeclaration>")
    alphabet += read_function_declaration(sfun);

  // parse the variable type information
  while (svar.substr(0,16) == "<varDeclaration>") {
    variables += read_variable_declaration(svar);
  }

  // parse the rules
  while (srul.substr(0,6) == "<rule>") {
    rules += read_rule_declaration(srul);
  }

  return alphabet + "\n" + variables + "\n" + rules + "\n";
}

void XMLReader :: sar(string &str, string from, string to) {
  int k = str.find(from);
  while (k != string::npos) {
    str.replace(k, from.length(), to);
    k = str.find(from);
  }
}

string XMLReader :: read_as_html(string filename) {
  string alphabet;
  string variables;
  string rules;

  transform_html = true;

  // read the file
  ifstream file(filename.c_str());
  string txt;
  while (!file.eof()) {
    string input;
    getline(file, input);
    txt += input;
  }

  // remove any whitespace
  int whitespace = txt.find(" ");
  while (whitespace != string::npos) {
    txt.erase(whitespace, 1); 
    whitespace = txt.find(" ", whitespace);
  }
  whitespace = txt.find(string(1,9));
  while (whitespace != string::npos) {
    txt.erase(whitespace, 1); 
    whitespace = txt.find(string(1,9), whitespace);
  }

  // get the individual parts
  string svar = get_first_part(txt, "variableTypeInfo");
  string sfun = get_first_part(txt, "functionSymbolTypeInfo");
  string srul = get_first_part(txt, "rules");

  // parse the alphabet
  while (sfun.substr(0,17) == "<funcDeclaration>") {
    string dec = read_function_declaration(sfun);
    int k = dec.find(":");
    string symb = dec.substr(0,k-1);
    string type = dec.substr(k+2,dec.length()-k-3);
    k = type.find("-->");
    if (k != string::npos) {
      string start = "[" + type.substr(1,k-3) + "]";
      type = start + " &#10230; " + type.substr(k+4);
    }
    sar(type, "->", "&rarr;");
    sar(type, "*", "&times;");
    alphabet += "<tr><td class=\"symbol func\">" + symb + "</td>" +
                "<td class=\"colon\">:</td><td class=\"type\">" +
                type + "</td></tr>\n";
  }

  // parse the variable type information
  while (svar.substr(0,16) == "<varDeclaration>") {
    string dec = read_variable_declaration(svar);
    int k = dec.find(":");
    string symb = dec.substr(0,k-1);
    string type = dec.substr(k+2,dec.length()-k-3);
    sar(type, "->", "&rarr;");
    variables += "<tr><td class=\"symbol var\">" + symb + "</td>" +
                 "<td class=\"colon\">:</td><td class=\"type\">" +
                 type + "</td></tr>\n";
  }

  // parse the rules
  while (srul.substr(0,6) == "<rule>") {
    string rule = read_rule_declaration(srul);
    sar(rule, "=>", "&rArr;");
    sar(rule, "/\\", "&lambda;");
    sar(rule, "*", "&middot;");
    int k = rule.find("&rArr;");
    string lhs = rule.substr(0,k-1);
    string rhs = rule.substr(k+7);
    rules += "<tr><td class=\"term lhs\">" + lhs +
             "</td><td class=\"rulestep\">&rArr;</td>" +
             "<td class=\"term rhs\">" + rhs + "</td></tr>\n";
  }

  filename = filename.substr(0,filename.length()-4);  // remove .xml
  int k = filename.find_last_of('/');
  if (k != string::npos) filename = filename.substr(k+1);

  return "<html><head><title>" + filename + "</title>\n"
    "<link rel=\"stylesheet\" href=\"afs.css\" type=\"text/css\" />\n"
    "</head><body>\n"
    "<h2 class=\"alphabet\">Alphabet</h2>\n"
    "<table class=\"alphabet\">" + alphabet + "</table>\n"
    "<h2 class=\"vars\">Variables</h2>\n"
    "<table class=\"vars\">" + variables + "</table>\n"
    "<h2 class=\"rules\">Rules</h2>\n"
    "<table class=\"rules\">" + rules + "</table>\n"
    "</body></html>\n";
}

string XMLReader :: get_part(string txt, string identifier,
                                  int start, int &end) {

  int a = txt.find("<" + identifier + ">", start);
  if (a == string :: npos) return "";
  int n = 1;
  end = -1;
  start = a+identifier.length()+2;

  while (true) {
    int b = txt.find("</" + identifier + ">", a+1);
    int c = txt.find("<" + identifier + ">", a+1);

    if (b == string::npos) return "!ERR!";

    if (c == string::npos || b < c) {
      n--;
      if (n == 0) {
        end = b + identifier.length()+2;
        return txt.substr(start,b-start);
      }
      a = b;
    }
    else {
      // c < b
      a = c;
      n++;
    }
  }
}

string XMLReader :: get_first_part(string txt, string identifier) {
  int end;
  return get_part(txt, identifier, 0, end);
}

string XMLReader :: fix_name(string name) {
  if (transform_html) {
    sar(name, "*", "&#42;");
    sar(name, "/\\", "&#x2227;");
    sar(name, "->", "&#x21e2;");
    sar(name, "=>", "&#x21e8;");
    return name;
  }

  for (int i = 0; i < name.length(); i++) {
    char c = name[i];
    if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
        ('0' <= c && c <= '9')) continue;
    string rep;
    if (c == '!') rep = "!fac";
    else if (c == '?') rep = "!question";
    else if (c == '.') rep = "!dot";
    else if (c == ':') rep = "!colon";
    else if (c == ';') rep = "!semicolon";
    else if (c == '*') rep = "!times";
    else if (c == '+') rep = "!plus";
    else if (c == '-') rep = "!minus";
    else if (c == '/') rep = "!div";
    else if (c == '$') rep = "!dollar";
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

string XMLReader :: read_type(string desc, int start, int &end,
                              bool brackets) {
  if (desc.substr(start,6) != "<type>") return "!ERR!";

  if (desc.substr(start+6,7) == "<basic>") {
    end = desc.find("</type>",start) + 6;
    string name = get_first_part(desc.substr(start), "basic");
    name = fix_name(name);
    return name;
  }

  if (desc.substr(start+6,7) != "<arrow>") return "!ERR!";

  string arrow = get_part(desc, "arrow", start, end);
  if (arrow == "" || end == string::npos ||
      desc.substr(end+1,7) != "</type>")
    return "!ERR!";
  end += 7;
  int a,b;
  string t1 = read_type(arrow, 0, a, true);
  string t2 = read_type(arrow, a+1, b, false);

  if (brackets) return "(" + t1 + " -> " + t2 + ")";
  else return t1 + " -> " + t2;
}

string XMLReader :: read_term(string desc, int start, int &end,
                              int brackets) {
  end = 0;

  /* variable */
  if (desc.substr(start,5) == "<var>") {
    string v = get_first_part(desc.substr(start), "var");
    if (v == "") return "!ERR!";
    end = start+v.length()+10;
    if (transform_html) v = "<span class=\"variable\">" + v + "</span>";
    return fix_name(v);
  }

  /* application */
  if (desc.substr(start,13) == "<application>") {
    int a = start+13, b;
    string t1 = read_term(desc, a, b, 2);
    a = b+1;
    string t2 = read_term(desc, a, b, 1);
    end = b+14;
    string ret = t1 + " * " + t2;
    if (brackets == 1) ret = "(" + ret + ")";
    return ret;
  }

  // function application
  if (desc.substr(start, 8) == "<funapp>") {
    string funapp = get_part(desc, "funapp", start, end);

    if (funapp == "") return "!ERR!";
    // get the function symbol
    string name = get_first_part(funapp, "name");
    if (name == "") return "!ERR!";
    if (transform_html) name = "<span class=\"constant\">" + name + "</span>";
    name = fix_name(name);
    bool arglist = false;
    string ret = name;
    int a = 0, b;
    // append all the arguments to ret
    string arg = get_part(funapp, "arg", a, b);
    while (arg != "") {
      if (arglist) ret += ", ";
      else { ret += "("; arglist = true; }
      int c;
      ret += read_term(arg, 0, c, 0);
      a = b+1;
      arg = get_part(funapp, "arg", a, b);
    }
    if (arglist) ret += ")";
    return ret;
  }

  // abstraction
  if (desc.substr(start,8) == "<lambda>") {
    int a = start+8, b;
    string varname = get_part(desc, "var", a, b);
    a = b+1;
    string type = "<type>" + get_part(desc, "type", a, b) + "</type>";
    a = b+1;
    if (varname == "" || type == "") return "!ERR!";
    varname = fix_name(varname);
    if (transform_html)
      varname = "<span class=\"variable\">" + varname + "</span>";
    string ty = read_type(type, 0, b);
    string t = read_term(desc, a, end, 0);
    if (desc.substr(end+1,9) != "</lambda>") return "!ERR!";
    end += 9;
    string ret = "/\\" + varname + ":" + ty + "." + t;
    if (brackets > 0) ret = "(" + ret + ")";
    return ret;
  }

  return "!ERR!";
}

string XMLReader :: read_function_declaration(string &txt) {
  if (txt.substr(0,17) != "<funcDeclaration>") return "!ERR!";
  txt = txt.substr(17);
  int end = txt.find("</funcDeclaration>");
  if (end == string::npos) {
    txt = "";
    return "!ERR!";
  }
  string decl = txt.substr(0,end);
  txt = txt.substr(end+18);

  string name = get_first_part(decl, "name");
  string type = get_first_part(decl, "typeDeclaration");
  if (name == "" || type == "") return "!ERR!";

  // find all types
  vector<string> types;
  int start = 0;
  while (start < type.length()) {
    string t = read_type(type, start, end);
    types.push_back(t);
    start = end+1;
  }
  string typedec;
  if (types.size() == 1) typedec = types[0];
  else {
    typedec = "(";
    for (int k = 0; k < types.size()-1; k++)
      typedec += types[k] + " * ";
    typedec = typedec.substr(0,typedec.length()-3) + ") --> " +
              types[types.size()-1];
  }

  name = fix_name(name);

  return name + " : " + typedec + "\n";
}

string XMLReader :: read_variable_declaration(string &txt) {
  if (txt.substr(0,16) != "<varDeclaration>") return "!ERR!";
  txt = txt.substr(16);
  int end = txt.find("</varDeclaration>");
  if (end == string::npos) {
    txt = "";
    return "!ERR!";
  }
  string decl = txt.substr(0,end);
  txt = txt.substr(end+17);

  string name = get_first_part(decl, "var");
  int k = decl.find("<type>"), l;
  if (k == string::npos) k = 0;
  string type = read_type(decl, k, l);
  name = fix_name(name);
  return name + " : " + type + "\n";
}

string XMLReader :: read_rule_declaration(string &txt) {
  if (txt.substr(0,6) != "<rule>") return "!ERR!";
  string lhs = get_first_part(txt, "lhs");
  string rhs = get_first_part(txt, "rhs");
  int end;

  string left = read_term(lhs, 0, end);
  string right = read_term(rhs, 0, end);

  // remove this rule declaration from txt
  end = txt.find("</rule>");
  if (end == string::npos) {
    txt = "";
    return "!ERR!";
  }
  txt = txt.substr(end+7);
  return left + " => " + right + "\n";
}

