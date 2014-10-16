#include "gtest/gtest.h"

#include "states.h"
#include <sstream>
#include <fstream>

using namespace std;
using namespace fst;

TEST(StateTest, ReadsDataFileWithoutException) {
	ifstream f("data/states.txt");
	vector<StateInfo> states = read_states(f);
}

TEST(StateTest, ReadsTrigram) {
	istringstream bigram("1 0 100 200");
	vector<StateInfo> states = read_states(bigram);
	EXPECT_EQ(100, states.front().fst);
	EXPECT_EQ(200, states.front().snd);
}

TEST(StateTest, ReadsBigram) {
	istringstream bigram("1 1 100");
	vector<StateInfo> states = read_states(bigram);
	EXPECT_EQ(100, states.front().snd);
}
