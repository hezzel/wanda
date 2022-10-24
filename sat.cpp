/**************************************************************************
   Copyright 2012 Cynthia Kop

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

#include "sat.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>

bool SatSolver :: solve(PFormula &formula) {
  formula = formula->conjunctive_form();

  if (formula == NULL) return false;
  if (formula->query_top()) return true;
  if (formula->query_bottom()) return false;
  if (formula->query_variable()) {
    unsigned int index = dynamic_cast<Var*>(formula)->query_index();
    vars.force_value(index, TRUE);
    return true;
  }
  if (formula->query_antivariable()) {
    unsigned int index = dynamic_cast<AntiVar*>(formula)->query_index();
    vars.force_value(index, FALSE);
    return true;
  }
  if (formula->query_disjunction()) {
    Or *dis = dynamic_cast<Or*>(formula);
    if (dis->query_number_children() == 0) return false;
    PFormula child = dis->query_child(0);
    if (child->query_variable()) {
      unsigned int index = dynamic_cast<Var*>(child)->query_index();
      vars.force_value(index, TRUE);
      return true;
    }
    else if (child->query_antivariable()) {
      unsigned int index = dynamic_cast<AntiVar*>(child)->query_index();
      vars.force_value(index, FALSE);
      return true;
    }
    else return false;    // shouldn't happen
  }

  if (formula->query_conjunction()) {  // the interesting case
    And *con = dynamic_cast<And*>(formula);
    // add the compulsary values to the formula
    for (unsigned int i = 0; i < vars.query_size(); i++) {
      if (vars.query_value(i) == TRUE) con->add_child(new Var(i));
      if (vars.query_value(i) == FALSE) con->add_child(new AntiVar(i));
    }
    // generate a file with all restrictions in the SAT competition format
    int size = con->query_number_children();
    FILE *fout = fopen("resources/input", "w");
    fprintf(fout, "p cnf %d %d\n", vars.query_size(), size);
    for (int i = 0; i < size; i++) {
      PFormula child = con->query_child(i);
      if (child->query_variable()) {
        fprintf(fout, "%d 0\n", dynamic_cast<Var*>(child)->query_index()+1);
      }
      else if (child->query_antivariable()) {
        fprintf(fout, "-%d 0\n", dynamic_cast<AntiVar*>(child)->query_index()+1);
      }
      else if (child->query_disjunction()) {
        Or *dis = dynamic_cast<Or*>(child);
        for (int i = 0; i < dis->query_number_children(); i++) {
          PFormula ch = dis->query_child(i);
          if (!ch->query_variable() && !ch->query_antivariable()) {
            // illegal format
            fclose(fout);
            return false;
          }
          int index;
          if (ch->query_variable()) {
            index = dynamic_cast<Var*>(ch)->query_index()+1;
          }
          else {
            fprintf(fout, "-");
            index = dynamic_cast<AntiVar*>(ch)->query_index()+1;
          }
          fprintf(fout, "%d", index);
          if (i == dis->query_number_children()-1) fprintf(fout, " 0\n");
          else fprintf(fout, " ");
        }
      }
      else {
        fclose(fout);
        return false;  // illegal format
      }
    }

    fclose(fout);

    // now run minisat on the generated file
    // system("./resources/timeout 20 ./resources/satsolver resources/input resources/output > /dev/null");
    system("./resources/timeout.sh 20 ./resources/satsolver resources/input resources/output > /dev/null");

    // and read the results!
    FILE *fin = fopen("resources/output", "r");
    char check[10];
    if (fin == NULL) return false;
    fscanf(fin, "%s", check);
    if (strcmp(check, "SAT") == 0) {
      int k;
      while (fscanf(fin, "%d", &k) == 1 && k != 0) {
        int var = (k < 0 ? -k-1 : k-1);
        if (k < 0) vars.force_value(var, FALSE);
        else vars.force_value(var, TRUE);
      }
      fclose(fin);
      system("rm resources/output");
      return true;
    }
    else {
      fclose(fin);
      system("rm resources/output");
      return false;
    }
  }

  // if it's something else, we can't handle it
  return false;
}

