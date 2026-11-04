#include "CastHistory/HistoryIR.h"

#include <cassert>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

using namespace cast_history;

namespace {

struct OperationRecord {
  ObsHistPtr attachPath;
  std::string pointerType;
  std::string accessType;
  std::string kind;
  std::string localGuard;
  std::string detail;
  SourceSpan operationSite;
  std::string originKind;
  std::string fieldEntity;
  std::string fieldPath;
  std::string calleeExpr;
  std::string signatureKey;
};

TypeAtom ptr(const char *text) {
  TypeAtom t;
  t.text = text;
  t.canonicalText = text;
  t.isPointer = true;
  t.kind = std::string(text) == "void *" ? TypeKind::VoidPointer
                                           : TypeKind::OtherPointer;
  return t;
}

ObsHistPtr chain(std::initializer_list<TypeAtom> types) {
  ObsHistPtr result;
  for (const TypeAtom &type : types)
    result = result ? mkObsSeq(result, mkObsAsType(type))
                    : mkObsAsType(type);
  return result;
}

std::string operationKey(const OperationRecord &use) {
  std::ostringstream os;
  os << use.operationSite.file << ':' << use.operationSite.line << ':'
     << use.operationSite.column << '|'
     << use.pointerType << '|'
     << use.accessType << '|'
     << use.kind << '|'
     << use.localGuard << '|'
     << use.detail << '|'
     << use.originKind << '|'
     << use.fieldEntity << '|'
     << use.fieldPath << '|'
     << use.calleeExpr << '|'
     << use.signatureKey;
  return os.str();
}

OperationRecord operation(const ObsHistPtr &attachment) {
  OperationRecord out;
  out.attachPath = attachment;
  out.pointerType = "char *";
  out.accessType = "char";
  out.kind = "deref-readwrite";
  out.localGuard = "<always>";
  out.detail = "deref-readwrite";
  out.operationSite = SourceSpan{"a.c", 17, 4};
  out.originKind = "same-tu-call";
  return out;
}

} // namespace

int main() {
  const ObsHistPtr shortAttach = chain({ptr("void *"), ptr("int *")});
  const ObsHistPtr longAttach = chain({ptr("void *"), ptr("int *"),
                                       ptr("void *"), ptr("char *"),
                                       ptr("void *"), ptr("int *")});

  OperationRecord first = operation(shortAttach);
  OperationRecord replay = operation(longAttach);

  // The normal C6g5q bug was that these were different because attachment
  // history had become part of pointer-use SCC membership.  C6g5r keys the
  // immutable operation leaf instead.
  assert(operationKey(first) == operationKey(replay));
  std::set<std::string> operations;
  assert(operations.insert(operationKey(first)).second);
  assert(!operations.insert(operationKey(replay)).second);

  // After cast quiescence, the stored report attachment is the frozen target
  // formal summary, so replay cannot manufacture a longer recursive chain.
  const ObsHistPtr frozenTarget =
      chain({ptr("void *"), ptr("int *"), ptr("void *"), ptr("char *")});
  first.attachPath = frozenTarget;
  replay.attachPath = frozenTarget;
  assert(ObsFormatter::render(first.attachPath) ==
         ObsFormatter::render(replay.attachPath));
  assert(ObsFormatter::render(first.attachPath).find("char *") !=
         std::string::npos);

  // Distinct source operation sites remain distinct, preserving relation- and
  // source-sensitive operation union.
  OperationRecord distinct = operation(frozenTarget);
  distinct.operationSite = SourceSpan{"a.c", 23, 4};
  assert(operationKey(distinct) != operationKey(first));

  std::cout << "C6g5r pointer-use operation identity smoke: OK\n";
}
