import base_messages;

namespace quisp::messages;

packet LinkAllocationUpdateMessage extends Header {
  unsigned long activeLinkAllocations[] @appender(appendActiveLinkAllocation) @getter(getActiveLinkAllocations) @sizeGetter(getActiveLinkAllocationCount);
  unsigned long nextLinkAllocations[] @appender(appendNextLinkAllocation) @getter(getNextLinkAllocations) @sizeGetter(getNextLinkAllocationCount);
  int random_number @setter(setRandomNumber) @getter(getRandomNumber);
}

packet RejectLinkAllocationUpdateMessage extends Header {}

packet LinkAllocationUpdateNotifier extends Header {
  unsigned long ruleset_id @setter(setRuleSetId) @getter(getRuleSetId);
  int stack_of_NeighboringQNodeIndices[];
}

