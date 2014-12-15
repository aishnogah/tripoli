/*
 * states.cpp
 *
 *  Created on: Oct 15, 2014
 *      Author: ara
 */

#include "tripoli.h"
#include <iostream>
#include <vector>

using namespace std;
using namespace fst;

vector<StateInfo> read_states(istream& f) {
	if (!f) {
		throw string("Could not open stream");
	}
	string line;
	vector<StateInfo> lines;
	while (getline(f, line)) {
		StateInfo s;
		istringstream l(line);
		// Throw away the first token, which is the id
		l.ignore(numeric_limits<streamsize>::max(), ' ');
		int arctag;
		l >> arctag;
		s.tag = StateTag(arctag);
		// Later on, these values are checked in the PDTInfo constructor
		switch(s.tag) {
		case StateTag::TRIGRAM_STATE:
			// URGENT TODO: Check both what the application assumes is
			// meant by fst and snd and what is given in the input file
			l >> s.fst;
			l >> s.snd;
			break;
		case StateTag::BIGRAM_STATE:
			l >> s.fst;
			s.snd = -1;
			break;
		default:
			// Since these are used in hashing, make sure there is no junk
			s.fst = -1;
			s.snd = -1;
		}
		lines.push_back(s);
	}
	return lines;
}
