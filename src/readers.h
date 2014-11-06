/*
 * readers.h
 *
 *  Created on: Nov 6, 2014
 *      Author: ara
 */

#ifndef READERS_H_
#define READERS_H_

#include <string>
#include <vector>

#include "tripoli.h"

using std::string;
using std::vector;

namespace fst {

template <typename T>
bool ReadIntVectors(const string &filename, vector<vector<T>> *vectors);
bool ReadNumberedStrings(const string &filename, vector<string> *strings);
bool ReadSymbolFile(const string& filename, Symbol *max_term, Symbol *max_preterm, Symbol *max_nonterm);
Grammar *ReadGrammar(const string symbolfile, const string rulefile, const string labelfile);

}

#endif /* READERS_H_ */
