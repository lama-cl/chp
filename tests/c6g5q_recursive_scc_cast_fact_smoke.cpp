#include "CastHistory/HistoryIR.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace cast_history;

namespace {
struct Step {
  std::string op;
  std::string guard;
};
struct Facts {
  bool supported = true;
  bool truncated = false;
  std::set<std::string> values;
};

TypeAtom ptr(const char *text) {
  TypeAtom t;
  t.text = text;
  t.canonicalText = text;
  t.isPointer = true;
  if (std::string(text) == "void *") {
    t.kind = TypeKind::VoidPointer;
    t.isVoidPointer = true;
  } else if (std::string(text) == "unsigned char *") {
    t.kind = TypeKind::CharPointer;
    t.isCharPointer = true;
  } else {
    t.kind = TypeKind::OtherPointer;
    t.pointeeCanonicalText = text;
  }
  return t;
}

std::string guardKey(std::vector<Guard> guards) {
  if (guards.empty()) return "<always>";
  std::vector<std::string> parts;
  for (const Guard &g : guards)
    parts.push_back((g.kind.empty() ? "guard" : g.kind) + "\x1f" +
                    (g.text.empty() ? "<empty>" : g.text));
  std::sort(parts.begin(), parts.end());
  parts.erase(std::unique(parts.begin(), parts.end()), parts.end());
  std::string out;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i) out += "&&";
    out += parts[i];
  }
  return out;
}

void append(std::vector<Step> &path, std::string op, std::string guard) {
  if (!path.empty() && path.back().op == op && path.back().guard == guard &&
      op.rfind("ptrarith:", 0) == 0)
    return;
  path.push_back({std::move(op), std::move(guard)});
}

Facts collect(const ObsHistPtr &root) {
  Facts facts;
  const auto expand = [&](auto &&self, const ObsHistPtr &node,
                          const std::vector<Guard> &guards)
      -> std::vector<std::vector<Step>> {
    if (!facts.supported || !node) return node ? std::vector<std::vector<Step>>{}
                                                   : std::vector<std::vector<Step>>{{}};
    if (node->kind == ObsKind::AsType || node->kind == ObsKind::PtrArith) {
      std::vector<Step> p;
      append(p, std::string(node->kind == ObsKind::AsType ? "cast:" : "ptrarith:") +
                    node->ty.semanticKey(),
             guardKey(guards));
      return {p};
    }
    if (node->kind == ObsKind::Seq) {
      auto lhs = self(self, node->first, guards);
      auto rhs = self(self, node->second, guards);
      std::vector<std::vector<Step>> out;
      for (const auto &a : lhs) for (const auto &b : rhs) {
        auto p = a;
        for (const Step &s : b) append(p, s.op, s.guard);
        out.push_back(std::move(p));
      }
      return out;
    }
    if (node->kind == ObsKind::Parallel) {
      std::vector<std::vector<Step>> out;
      for (const auto &arm : node->arms) {
        auto paths = self(self, arm, guards);
        out.insert(out.end(), paths.begin(), paths.end());
      }
      return out;
    }
    if (node->kind == ObsKind::Join) {
      std::vector<std::vector<Step>> out;
      for (const ObsAlt &alt : node->alts) {
        auto next = guards;
        next.push_back(alt.guard);
        auto paths = self(self, alt.hist, next);
        out.insert(out.end(), paths.begin(), paths.end());
      }
      return out;
    }
    facts.supported = false;
    return {};
  };

  for (const auto &path : expand(expand, root, {})) {
    for (const Step &s : path)
      facts.values.insert("atom|guard=" + s.guard + "|op=" + s.op);
    for (size_t i = 1; i < path.size(); ++i)
      facts.values.insert("edge|from-guard=" + path[i - 1].guard +
                          "|from-op=" + path[i - 1].op +
                          "|to-guard=" + path[i].guard +
                          "|to-op=" + path[i].op);
  }
  return facts;
}

bool addsNew(const std::set<std::string> &state, const Facts &candidate) {
  if (!candidate.supported || candidate.truncated) return true; // fail open
  for (const std::string &fact : candidate.values)
    if (!state.count(fact)) return true;
  return false;
}

void commit(std::set<std::string> &state, const Facts &facts) {
  state.insert(facts.values.begin(), facts.values.end());
}

ObsHistPtr chain(std::initializer_list<TypeAtom> types) {
  ObsHistPtr result;
  for (const TypeAtom &t : types)
    result = result ? mkObsSeq(result, mkObsAsType(t)) : mkObsAsType(t);
  return result;
}
} // namespace

int main() {
  const TypeAtom byte = ptr("unsigned char *");
  const ObsHistPtr guardedCore = mkObsJoin({
      {Guard{"tag", "if-then"}, mkObsPtrArith(byte)},
      {Guard{"!(tag)", "if-else"}, mkObsPtrArith(byte)}});
  const ObsHistPtr prefix = mkObsPtrArith(byte);
  const ObsHistPtr firstTransfer = mkObsSeq(prefix, guardedCore);

  // p127: the first recursive composition adds one unguarded-to-guarded
  // transition; replaying the same local guarded transformer adds no fact.
  std::set<std::string> p127State = collect(guardedCore).values;
  Facts firstFacts = collect(firstTransfer);
  assert(firstFacts.supported && addsNew(p127State, firstFacts));
  commit(p127State, firstFacts);
  const ObsHistPtr replay = mkObsSeq(prefix, mkObsParallel({guardedCore, firstTransfer}));
  Facts replayFacts = collect(replay);
  assert(replayFacts.supported && !addsNew(p127State, replayFacts));

  // recursive-mutual-three: a full cross-function cast sequence contributes
  // distinct ordered transitions and therefore is not suppressed merely because
  // individual type atoms already occur in direct local summaries.
  const ObsHistPtr directAtoms = mkObsParallel({
      mkObsAsType(ptr("void *")), mkObsAsType(ptr("int *")),
      mkObsAsType(ptr("char *")), mkObsAsType(ptr("double *"))});
  std::set<std::string> threeState = collect(directAtoms).values;
  const ObsHistPtr threeCycle = chain({ptr("void *"), ptr("int *"),
                                       ptr("void *"), ptr("char *"),
                                       ptr("void *"), ptr("double *"),
                                       ptr("void *")});
  Facts threeFacts = collect(threeCycle);
  assert(threeFacts.supported && addsNew(threeState, threeFacts));
  const std::string rendered = ObsFormatter::render(threeCycle);
  assert(rendered.find("int *") != std::string::npos);
  assert(rendered.find("char *") != std::string::npos);
  assert(rendered.find("double *") != std::string::npos);

  // Callback/wrapper structures are outside the finite cast-fact domain and
  // must preserve the C6g5k path by failing open.
  Facts wrapped = collect(mkObsCallWrap("cb", mkObsAsType(ptr("int *"))));
  assert(!wrapped.supported && addsNew({}, wrapped));

  std::cout << "C6g5q recursive-SCC cast-fact smoke: OK\n";
}
