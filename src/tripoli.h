// ben.h
// author: bsnyder@cs.wisc.edu (Benjamin Snyder)

#ifndef TRIPOLI_TRIPOLI_H__
#define TRIPOLI_TRIPOLI_H__

#include <cstdlib>
#include <cstring>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <utility>
#include <tuple>
#include <vector>
#include <memory>
#include <functional>

using std::string;
using std::invalid_argument;
using std::unordered_map;
using std::unordered_set;
using std::pair;
using std::make_pair;
using std::tuple;
using std::vector;
using std::unique_ptr;
using std::set_union;
using std::hash;

#include <fst/fst.h>
#include <fst/mutable-fst.h>
#include <fst/matcher.h>
#include <fst/compose.h>
#include <fst/extensions/pdt/compose.h>
#include <fst/util.h>
#include <fst/filter-state.h>

namespace fst {

typedef int Label;  // arc label
typedef int Symbol; // grammar symbol
typedef int StateId;
typedef int RuleId;
typedef vector<Symbol> Rule;  // grammar rule

template <class Arc>
class RuleArc : public Arc {
public:
  typedef typename Arc::Weight Weight;

  RuleArc(Label i, Label o, const Weight& w, StateId s, RuleId r = -1)
          : Arc(i, o, w, s), rule(r) {}
  RuleArc() {}

  RuleId rule;
};

enum ReachType {
  REACH_UNKNOWN,
  REACH_YES,
  REACH_NO = 0
};

class Grammar {
public:
  Grammar(Symbol max_term, Symbol max_preterm, Symbol max_nonterm,
              vector<Rule> rules = vector<Rule>(), vector<Symbol> labels_to_symbols = vector<Symbol>())
          : max_term_(max_term), max_preterm_(max_preterm), max_nonterm_(max_nonterm),
            labels_to_symbols_(labels_to_symbols) {

    if (max_term <= 0)
      invalid_argument("max_term should be > 0");
    if (max_preterm != max_term*2)
      invalid_argument("max_preterm should be 2 * max_term");
    if (max_nonterm <= max_preterm)
      invalid_argument("max_nonterm should be > max_preterm");

    SetRules(rules);

    for (Symbol pt = max_term + 1; pt <= max_preterm; ++pt) {
      Symbol t = ToTerm(pt);
      symbol_reach_[pt][t] = REACH_YES;
    }
  }

  bool IsTerm(Symbol s) { return s > 0 && s <= max_term_; }
  bool IsPreterm(Symbol s) { return s > max_term_ && s <= max_preterm_; }
  bool IsNonterm(Symbol s) { return s > max_preterm_ && s <= max_nonterm_; }
  Symbol LabelToSymbol(Label l) { return labels_to_symbols_[l]; }

  Symbol ToPreterm(Symbol t) {
    if (t <= max_term_)
      return max_term_ + t;
    else return -1;
  }
  Symbol ToTerm(Symbol pt) {
    if (pt > max_term_ && pt <= max_preterm_)
      return pt - max_term_;
    else return -1;
  }

  bool SymbolCanReach(Symbol nonterm, Symbol term) {
    if (IsTerm(nonterm) || !IsTerm(term))
      invalid_argument("SymbolCanReach: nonterm must not be term, and term must be term");

    ReachType cached = symbol_reach_[nonterm][term];
    if (cached != REACH_UNKNOWN)
      return cached;

    symbol_reach_[nonterm][term] = REACH_NO;  // prevent infinite loops
    vector<Symbol> replacements = replacement_symbols_[nonterm];
    for (vector<Symbol>::const_iterator it = replacements.begin();
                                        it != replacements.end(); ++it) {
      if (SymbolCanReach(*it, term)) {
        symbol_reach_[nonterm][term] = REACH_YES;
        return true;
      }
    }
    return false;
  }

  bool RuleCanReach(RuleId r, Symbol term) {
    //TODO cache separately for speed-up
    return SymbolCanReach(rules_[r][1], term);
  }

  void ValidateRule(Rule rule) {
    if (rule.size() < 2)
      invalid_argument("invalid rule: must have at least two symbols");
    if (rule.size() == 2 && IsPreterm(rule[0]) && IsTerm(rule[1])) { // unary terminal production
      Symbol pterm = rule[0];
      Symbol term = rule[1];
      if (ToTerm(pterm) != term)
        invalid_argument("invalid rule: preterm and term do not match in unary production");
    }
    else {  // not a unary terminal production
      Rule::const_iterator it = rule.begin();
    if (!IsNonterm(*it))
      invalid_argument("invalid rule: non-unary production must have non-terminal left-hand symbol");
    for (++it; it != rule.end(); ++it)
      if (!(IsNonterm(*it) || IsPreterm(*it)))
        invalid_argument("invalid rule: non-unary production must not have terminal right-hand symbols");
    }
  }

  void SetRules(const vector<Rule>& rules) {
    rules_ = rules;
    replacement_symbols_ = vector<vector<Symbol> >(max_nonterm_+1, vector<Symbol>());
    for (ssize_t i = 0; i < rules.size(); ++i) {
      Rule rule = rules[i];
      ValidateRule(rule);
      Symbol lsym = rule[0];
      Symbol rsym = rule[1];
      vector<Symbol> repls = replacement_symbols_[lsym];
      if (std::find(repls.begin(), repls.end(), rsym) == repls.end())
        repls.push_back(rsym);
    }
  }

private:
  Symbol max_term_;    // assume first terminal is 1 (0 reserved for epsilon)
  Symbol max_preterm_; // assume min_preterm_ is max_term_ + 1, assume max_preterm_ == max_term_ * 2
  Symbol max_nonterm_; // assume min_nonterm_ is max_preterm_ + 1
  vector<Symbol> labels_to_symbols_;
  vector<Rule> rules_;
  vector<vector<ReachType> > symbol_reach_;
  vector<vector<Symbol> > replacement_symbols_;
  // replacement_symbols_[s] is a vector of symbols which appear as the left-most symbol of the RHS of a production from s
};

enum StateTag {
  TRIGRAM_STATE = 0,
  BIGRAM_STATE = 1,
  UNIGRAM_STATE = 2,
  DUMMY_STATE = 3,
  PORTAL_STATE = 4
};

enum ArcTag {
  DUMMY_ARC = -1,
  PORTAL_ARC = -2,
  LEXICAL_BACKOFF_ARC = -3,
  SYNTACTIC_BACKOFF_ARC = -4,
};

struct StateInfo {
  StateTag tag;
  Symbol fst;
  Symbol snd;
};


class TripoliFilterState {
public:
  TripoliFilterState() : no_state_flag_(false) {
    states_.reserve(3);
    labels_.reserve(3);
  }
  TripoliFilterState(const vector<StateId> &states, const vector<Label> &labels, const set<RuleId> &disallowed)
          : no_state_flag_(false), states_(states), labels_(labels), disallowed_(disallowed) {}

  TripoliFilterState(bool no_state_flag) : no_state_flag_(no_state_flag) {}

   TripoliFilterState GenerateAddState(StateId state, const set<RuleId> &disallowed) const {
     set<RuleId> new_disallowed;
     set_union(disallowed_.cbegin(), disallowed_.cend(), disallowed.cbegin(), disallowed.cend(),
             std::inserter(new_disallowed, new_disallowed.end()));

     vector<StateId> new_states(states_);
     new_states.push_back(state);
     return TripoliFilterState(new_states, labels_, new_disallowed);
   }
  TripoliFilterState GenerateAddLabel(Label label, const set<RuleId> &disallowed) const {
    set<RuleId> new_disallowed;
    set_union(disallowed_.cbegin(), disallowed_.cend(), disallowed.cbegin(), disallowed.cend(),
           std::inserter(new_disallowed, new_disallowed.end()));

    vector<Label> new_labels(labels_);
    new_labels.push_back(label);
    return TripoliFilterState(states_, new_labels, new_disallowed);
  }

  bool Contains(RuleId r) const {
    return disallowed_.find(r) != disallowed_.end();
  }

  static const TripoliFilterState &NoState() { return no_state_;}

  template <typename T>
  static size_t VecHash(vector<T> vec) {
    size_t h = 0;
    for (typename vector<T>::const_iterator it = vec.cbegin();
         it != vec.cend(); ++it) {
      h ^= h << 1 ^ *it;
    }
    return h;
  }

  size_t Hash() const {
    size_t h1 = VecHash<StateId>(states_);
    size_t h2 = VecHash<Label>(labels_);
    const int lshift = 5;
    const int rshift = CHAR_BIT * sizeof(size_t) - 5;
    return h1 << lshift ^ h1 >> rshift ^ h2;
  }

  bool operator==(const TripoliFilterState &f) const {
    return (f.no_state_flag_ && no_state_flag_) || (f.states_ == states_ && f.labels_ == labels_);
  }

  bool operator!=(const TripoliFilterState &f) const {
    return !(f.no_state_flag_ && no_state_flag_) && (f.states_ != states_ || f.labels_ != labels_);
  }

private:
  bool no_state_flag_;
  vector<StateId> states_;
  vector<Label> labels_;
  set<RuleId> disallowed_;

  static TripoliFilterState no_state_;

  template <class M1, class M2> friend class TripoliComposeFilter;
};

struct FilterStateHash {
  size_t operator()(const TripoliFilterState &f) { return f.Hash(); }
};

struct StateInfoHash {
  size_t operator()(StateInfo const& si) const {
    hash<Symbol> hash_fn;
    size_t h1 = hash_fn(si.fst);
    size_t h2 = hash_fn(si.snd);
    return h1 << 5 ^ h1 >> (CHAR_BIT * sizeof(size_t) - 5) ^ h2;
  }
};

struct StateInfoEquals {
  bool operator()(StateInfo const& si1, StateInfo const& si2) const {
    return (si1.tag == si2.tag && si1.fst == si2.fst && si1.snd == si2.snd);
  }
};

template <class F>
class PDTInfo {
public:
  typedef F PDT;
  typedef typename F::Arc Arc;
  typedef typename Arc::Weight Weight;

  PDTInfo(Grammar &grammar, PDT &pdt, vector<StateInfo> &state_info)
          : grammar(grammar),
            pdt_(pdt),
            state_info_(state_info) {

    StateId state = 0;
    for (vector<StateInfo>::const_iterator it = state_info.begin();
         it != state_info.end();
         ++it, ++state) {
      StateInfo si = *it;

      switch (si.tag) {
        case TRIGRAM_STATE:
          if (!(grammar.IsTerm(si.fst) && grammar.IsTerm(si.snd)))
            invalid_argument("invalid StateInfo for trigram state: " + std::to_string(state));
          collect_rules(state);
          state_index_[si] = state;
          break;

        case BIGRAM_STATE:
          if (!(grammar.IsTerm(si.fst) && si.snd == 0))
            invalid_argument("invalid StateInfo for bigram state: " + std::to_string(state));
          collect_rules(state);
          state_index_[si] = state;
          break;

        case UNIGRAM_STATE:
          if (!(si.fst == 0 && si.snd == 0))
            invalid_argument("invalid StateInfo for unigram state: " + std::to_string(state));
          collect_unigram_rules(state);
          state_index_[si] = state;
          break;

        case DUMMY_STATE:
          if (!(si.fst == 0 && si.snd == 0))
            invalid_argument("invalid StateInfo for dummy state: " + std::to_string(state));
          break;
        case PORTAL_STATE:
          if (!(si.fst == 0 && si.snd == 0))
            invalid_argument("invalid StateInfo for portal state: " + std::to_string(state));
      }
    }
  }

  // TODO Why can't this be const?
  set<RuleId> &GetContextRuleSet(StateId s) {
    return seen_rules_[s];
  }

  // TODO Why can't this be const?
  set<RuleId> &GetUnigramRuleSet(Label l) {
    return unigram_rules_[l];
  }

  StateInfo &GetStateInfo(StateId s) const {
    return state_info_[s];
  }

private:
  void collect_rules(StateId s) {
    set<RuleId> &rules = seen_rules_[s];
    for (ArcIterator<PDT> aiter(pdt_, s);
         !aiter.Done();
         aiter.Next()) {
      const Arc &arc = aiter.Value();
      rules.insert(arc.rule);
    }
  }
  void collect_unigram_rules(StateId s) {
    for (ArcIterator<PDT> aiter(pdt_, s);
         !aiter.Done();
         aiter.Next()) {
      const Arc &arc = aiter.Value();
      Label label = arc.ilabel;
      if (label == 0)
        continue;
      unigram_rules_[label].insert(arc.rule);
    }
  }

  PDT pdt_;
  vector<StateInfo> state_info_;  // maps StateId to state-info
  unordered_map<StateInfo, StateId, StateInfoHash, StateInfoEquals> state_index_; // maps (context) state-info to StateId
  unordered_map<StateId, set<RuleId>> seen_rules_; // maps stateId (context state) to observed rules
  unordered_map<Label, set<RuleId>> unigram_rules_; // maps arc-Label (pop) to set of rules seen with that context from unigram state
//  unordered_map<FilterState, set<RuleId>, FilterStateHash> cached_filter_sets_;

public:
  Grammar grammar;
};


template <class M1, class M2>
class TripoliComposeFilter {
public:
  typedef typename M1::FST FST;
  typedef typename M2::FST PDT;
  typedef typename PDT::Arc Arc;
  typedef M1 Matcher1;
  typedef M2 Matcher2;
  typedef TripoliFilterState FilterState;

  TripoliComposeFilter(const FST &fst, const PDT &pdt, const PDTInfo<PDT> &pdt_info, M1 *matcher1 = 0, M2 *matcher2 = 0)
          : matcher1_(matcher1 ? matcher1 : new M1(fst, MATCH_OUTPUT)),
            matcher2_(matcher2 ? matcher2 : new M2(pdt, MATCH_INPUT)),
            fst_(matcher1_->GetFst()),
            pdt_(matcher2_->GetFst()),
            pdt_info_(pdt_info),
            s1_(kNoStateId),
            s2_(kNoStateId),
            f_(kNoStateId) { TripoliFilterState::no_state_ = FilterState(true); }

  TripoliComposeFilter(const TripoliComposeFilter<M1, M2> &filter, bool safe = false)
          : matcher1_(filter.matcher1_->Copy(safe)),
            matcher2_(filter.matcher2_->Copy(safe)),
            fst_(matcher1_->GetFst()),
            pdt_(matcher2_->GetFSt()),
            pdt_info_(filter.pdt_info_),
            s1_(kNoStateId),
            s2_(kNoStateId),
            f_(kNoStateId) { TripoliFilterState::no_state_ = FilterState(true); }

  ~TripoliComposeFilter() {
    delete matcher1_;
    delete matcher2_;
  }

  FilterState Start() const { return FilterState(); }

  void SetState(StateId s1, StateId s2, const FilterState &f) {
    if (s1_ == s1 && s2_ == s2 && f == f_)
      return;
    s1_ = s1;
    s2_ = s2;
    f_ = f;
  }

  const FilterState FilterArc(Arc *arc1, Arc *arc2) const {
    RuleId r = arc2->rule;
    switch (r) {
      case LEXICAL_BACKOFF_ARC: {
        set<RuleId> &disallowed = pdt_info_.GetContextRuleSet(s2_);
        return f_.GenerateAddState(s2_, disallowed);
      }
      case SYNTACTIC_BACKOFF_ARC: {
        set<RuleId> &disallowed = pdt_info_.GetUnigramRuleSet(arc2->ilabel);
        return f_.GenerateAddLabel(arc2->ilabel, disallowed);
      }
      case DUMMY_ARC:
      case PORTAL_ARC:
        return f_;
    }
    if (f_.Contains(r))
      return TripoliFilterState::NoState();

    if (!pdt_info_.grammar.RuleCanReach(r, arc1->olabel))
      return TripoliFilterState::NoState();

    return f_;
  }

private:
  Matcher1 *matcher1_;
  Matcher2 *matcher2_;
  const FST &fst_;
  const PDT &pdt_;
  const PDTInfo<PDT> &pdt_info_;
  StateId s1_;
  StateId s2_;
  TripoliFilterState f_;
  vector<set<Label>> fst_state_labels_;

  void operator=(const TripoliComposeFilter<M1, M2> &); // disallow
};

};
#endif   // TRIPOLI_TRIPOLI_H__
