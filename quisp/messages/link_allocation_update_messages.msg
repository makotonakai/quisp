import base_messages;

namespace quisp::messages;

packet LinkAllocationUpdateRequest extends Header {
  unsigned long stack_of_ActiveLinkAllocations[] @appender(appendActiveLinkAllocation) @getter(getActiveLinkAllocations) @sizeGetter(getActiveLinkAllocationCount);
  int random_number @setter(setRandomNumber) @getter(getRandomNumber);
}

packet LinkAllocationUpdateResponse extends Header {
  unsigned long stack_of_ActiveLinkAllocations[] @appender(appendActiveLinkAllocation) @getter(getActiveLinkAllocations) @sizeGetter(getActiveLinkAllocationCount);
}


packet RejectLinkAllocationUpdateRequest extends Header {
  unsigned long stack_of_ActiveLinkAllocations[] @appender(appendActiveLinkAllocation) @getter(getActiveLinkAllocations) @sizeGetter(getActiveLinkAllocationCount);
}

packet InternalNeighborAddressesMessage extends Header {
  unsigned long ruleset_id @setter(setRuleSetId) @getter(getRuleSetId);
  int stack_of_NeighboringQNodeIndices[];
}


