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
		switch(s.tag) {
		// TODO check that this is the right ordering assumptions of fst and snd in the code
		// TODO also check the initialization value
		case StateTag::TRIGRAM_STATE:
			l >> s.fst;
			l >> s.snd;
			break;
		case StateTag::BIGRAM_STATE:
			l >> s.snd;
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
