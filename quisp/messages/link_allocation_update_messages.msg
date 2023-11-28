import base_messages;

namespace quisp::messages;

packet LinkAllocationUpdateRequest extends Header {
  unsigned long stack_of_ActiveLinkAllocations[];
  int random_number @setter(setRandomNumber) @getter(getRandomNumber);
}

packet LinkAllocationUpdateResponse extends Header {
  unsigned long stack_of_ActiveLinkAllocations[];
}


packet RejectLinkAllocationUpdateRequest extends Header {
  unsigned long stack_of_ActiveLinkAllocations[];
}

packet InternalNeighborAddressesMessage extends Header {
  unsigned long RuleSet_id  @setter(setRuleSetId) @getter(getRulesetId);
  int stack_of_NeighboringQNodeIndices[];
}


