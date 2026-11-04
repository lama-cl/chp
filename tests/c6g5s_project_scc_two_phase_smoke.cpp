// Standalone model test for C6g5s's finite project SCC domains.
#include <cassert>
#include <set>
#include <string>

int main() {
  // The project cast domain records semantic facts, not nested report trees.
  std::set<std::string> castFacts{
      "atom|guard=always|op=astype:int *",
      "edge|from-guard=always|from-op=astype:int *|to-guard=always|to-op=astype:char *"};
  const std::set<std::string> replay = castFacts;
  bool replayAddsFact = false;
  for (const auto &fact : replay)
    replayAddsFact |= castFacts.insert(fact).second;
  assert(!replayAddsFact);

  // A genuinely new transition remains visible; C6g5s must not truncate a
  // distinct three-function cast cycle merely because it is recursive.
  assert(castFacts.insert(
      "edge|from-guard=always|from-op=astype:char *|to-guard=always|to-op=astype:double *").second);

  // Phase-2 operation membership intentionally excludes the growing report
  // attachment.  The same dereference seed arriving with another attachment
  // cannot schedule another pointer-use edge.
  std::set<std::string> operationKeys;
  const std::string stableKey =
      "param=0|site=g.c:7:3|kind=deref-readwrite|type=char *|guard=always|leaf=*pc += 1";
  assert(operationKeys.insert(stableKey).second);
  assert(!operationKeys.insert(stableKey).second);
  return 0;
}
