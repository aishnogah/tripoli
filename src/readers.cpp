/*
 * readers.cpp
 *
 *  Created on: Nov 6, 2014
 *      Author: ara
 */

#include <fst/fst.h>
#include "readers.h"
#include "tripoli.h"

using namespace std;
using namespace fst;

namespace fst {

template <typename T>
bool ReadIntVectors(const string& filename, vector<vector<T>> *vectors) {
  ifstream strm(filename.c_str());
  if (!strm) {
    LOG(ERROR) << "ReadIntVectors: Can't open file: " << filename;
    return false;
  }
  const int kLineLen = 8096;
  char line[kLineLen];
  size_t nline = 0;
  vectors->clear();
  while (strm.getline(line, kLineLen)) {
    ++nline;
    vector<char *> col;
    SplitToVector(line, "\n\t ", &col, true);
    // empty line or comment?
    if (col.size() == 0 || col[0][0] == '\0' || col[0][0] == '#')
      continue;

    bool err;
    vector<T> vec;
    for (size_t i = 0; i < col.size(); ++i) {
      T n = StrToInt64(col[i], filename, nline, false, &err);
      if (err) return false;
      vec.push_back(n);
    }
    vectors->push_back(vec);
  }
  return true;
};
template bool ReadIntVectors(const string&, vector<Rule>*);

bool ReadNumberedStrings(const string& filename, vector<string> *strings) {
  ifstream strm(filename.c_str());
  if (!strm) {
    LOG(ERROR) << "ReadNumberedStrings: Can't open file: " << filename;
    return false;
  }
  const int kLineLen = 8096;
  char s1[kLineLen];
  size_t nline = 0;
  strings->clear();
  int prevn = -1;
  bool err;
  char *s2;
  while (strm.getline(s1, kLineLen)) {
    ++nline;
    if ((s2 = strpbrk("\t ", s1)))
      *s2 = '\0';
    int64 n = StrToInt64(s1, filename, nline, false, &err);
    if (err) return false;
    if (n <= prevn) return false;
    for (; prevn < n; ++prevn)
      strings->push_back("");
    strings->push_back(s2);
  }
  return true;
}

bool ReadSymbolFile(const string& filename, Symbol *max_term,
        Symbol *max_preterm, Symbol *max_nonterm) {
  vector<string> symbols;
  ReadNumberedStrings(filename, &symbols);
  bool in_preterms = false;
  bool in_nonterms = false;
  size_t i = 1;
  for (; i < symbols.size(); ++i) {
    string sym = symbols[i];
    if (sym[0] == '_') {
      if (!in_preterms && !in_nonterms) {
        *max_term = i - 1;
        in_preterms = true;
      }
      if (!in_preterms)
        return false;
      if (strcmp(sym.c_str() + 1, symbols[i - *max_term].c_str()))
        return false;
    }
    else if (in_preterms) {
      *max_preterm = i - 1;
      in_preterms = false;
      in_nonterms = true;
    }
  }
  if (in_nonterms) {
    *max_nonterm = i;
    return true;
  }
  return false;
}

inline bool ReadLabelFile(const string& filename, vector<Symbol> *labels_to_symbols, const Symbol max_term) {
  vector<string> labels;
  ReadNumberedStrings(filename, &labels);
  labels_to_symbols->push_back(-1);
  for (size_t i = 1; i < labels.size(); ++i) {
    string label = labels[i];
    if (label[0] != '+' && label[0] != '-') {
      if (i > max_term)
        return false;
      labels_to_symbols->push_back(i);
    }
    else {
      if (i <= max_term) return false;
      char *sym_str = strpbrk(label.c_str(), "P");
      if (!sym_str) return false;
      bool err;
      Symbol sym = StrToInt64(sym_str, filename, i, false, &err);
      if (err) return false;
      labels_to_symbols->push_back(sym);
    }
  }
  return true;
}

Grammar *ReadGrammar(const string symbolfile, const string rulefile, const string labelfile) {
	Symbol max_term;
	Symbol max_preterm;
	Symbol max_nonterm;
	vector<Rule> rules;
	vector<Symbol> labels_to_symbols;
	if (!ReadIntVectors(rulefile, &rules))
		invalid_argument("cannot read rule file");
	if (!ReadSymbolFile(symbolfile, &max_term, &max_preterm, &max_nonterm))
		invalid_argument("cannot read grammar-symbols file");
	if (!ReadLabelFile(labelfile, &labels_to_symbols, max_term))
		invalid_argument("cannot read labels file");
	return new Grammar(max_term, max_preterm, max_nonterm, rules, labels_to_symbols);
}

};
