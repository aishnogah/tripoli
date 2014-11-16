//
//  main.cpp
//  triploli
//
//  Created by Benjamin Snyder on 10/5/14.
//  Copyright (c) 2014 Snyder. All rights reserved.
//

#include <iostream>

// fstcompile.cc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2005-2010 Google, Inc.
// Author: riley@google.com (Michael Riley)
// Modified: jpr@google.com (Jake Ratkiewicz) to use FstClass
//
// \file
// Creates binary FSTs from simple text format used by AT&T
// (see http://www.research.att.com/projects/mohri/fsm/doc4/fsm.5.html).

#include "tripoli-compile.h"
#include "tripoli.h"
#include "states.h"
#include "readers.h"
#include <fst/script/fst-class.h>
#include <fst/script/compile-impl.h>
#include <fst/extensions/pdt/compose.h>

DEFINE_bool(acceptor, false, "Input in acceptor format");
DEFINE_string(arc_type, "standard", "Output arc type");
DEFINE_string(fst_type, "vector", "Output FST type");
DEFINE_string(isymbols, "", "Input label symbol table");
DEFINE_string(osymbols, "", "Output label symbol table");
DEFINE_string(ssymbols, "", "State label symbol table");
DEFINE_bool(keep_isymbols, false, "Store input label symbol table with FST");
DEFINE_bool(keep_osymbols, false, "Store output label symbol table with FST");
DEFINE_bool(keep_state_numbering, false, "Do not renumber input states");
DEFINE_bool(allow_negative_labels, false, "Allow negative labels (not recommended; may cause conflicts)");

typedef typename fst::StdArc FstArc;
typedef typename fst::RuleArc<fst::StdArc> PdtArc;
typedef typename fst::VectorFst<PdtArc> StdVectorPdt;

int main(int argc, char **argv) {
  using fst::istream;
  using fst::ifstream;
  using fst::SymbolTable;

  const char *inputFilename = argv[1];
  const char *pdtFilename = argv[2];
  const char *labels = argv[3];
  const char *symbols = argv[4];
  const char *rules = argv[5];
  const char *states = argv[6];
  const char *parens = argv[7];
  const char *out_name = argv[8];

  istream *fstIstrm = new ifstream(inputFilename);
  istream *pdtIstrm = new ifstream(pdtFilename);
  const SymbolTable *isyms = 0, *osyms = 0, *ssyms = 0;

  fst::SymbolTableTextOptions opts;
  opts.allow_negative = false;

//  isyms = SymbolTable::ReadText(labels, opts);
//  osyms = isyms;

  bool accep = true;
  bool ikeep = false;
  bool okeep = false;
  bool allow_negative_labels = false;

  fst::FstCompiler<FstArc> fstCompiler(*fstIstrm, inputFilename, isyms, osyms, ssyms, accep, ikeep, okeep, allow_negative_labels);
  fst::PdtCompiler<PdtArc> pdtCompiler(*pdtIstrm, pdtFilename, isyms, osyms, ssyms, accep, ikeep, okeep, allow_negative_labels);

  StdVectorFst fst = fstCompiler.Fst();
  cout << "input FST compile..." << endl;

  StdVectorPdt pdt = pdtCompiler.Pdt();
  cout << "PDT compiled..." << endl;

  // Read the state file
  ifstream stateFile(states);
  vector<fst::StateInfo> stateInfo = read_states(stateFile);
  cout << "States read..." << endl;

  // Read the grammar file
  fst::Grammar *grammar = fst::ReadGrammar(symbols, rules, labels);

  // Read the parentheses file
  vector<pair<int64, int64> > parentheses;
  fst::ReadLabelPairs(parens, &parentheses, false);
  cout << "Parentheses read..." << endl;
  // TODO check parenthesis order matches what we need

  fst::PDTInfo<StdVectorPdt> pdtInfo(*grammar, pdt, stateInfo);

  typedef fst::ParenMatcher< StdVectorFst > FstMatcher;
  typedef fst::ParenMatcher< StdVectorPdt > PdtMatcher;
  FstMatcher matcher1(fst, MATCH_OUTPUT);
  PdtMatcher matcher2(pdt, MATCH_INPUT);
  fst::TripoliComposeFilter<FstMatcher, PdtMatcher> tripoliFilter(fst, pdt, pdtInfo, &matcher1, &matcher2);
  // where are the parens?

//  fst::script::VectorFstClass ofst(ifst->ArcType());
  fst::ComposeOptions composeOpts(true, fst::AUTO_FILTER); // Your filter goes here
  // TODO Duh the second argument is wrong
  // fst::script::Compose(*ifst, *ifst, &ofst, composeOpts);
  // ofst.Write(out_name);
}
