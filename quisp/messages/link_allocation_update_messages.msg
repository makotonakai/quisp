import base_messages;

namespace quisp::messages;

packet LinkAllocationUpdateRequest extends Header {
  int srcAddr @setter(setSrcAddr) @getter(getSrcAddr);
  int destAddr @setter(setDestAddr) @getter(getDestAddr);
  unsigned long stack_of_ActiveLinkAllocations[];
  int random_number @setter(setRandomNumber) @getter(getRandomNumber); 
}

packet LinkAllocationUpdateResponse extends Header {
  int srcAddr @setter(setSrcAddr) @getter(getSrcAddr);
  int destAddr @setter(setDestAddr) @getter(getDestAddr);
  unsigned long stack_of_ActiveLinkAllocations[];
}


packet RejectLinkAllocationUpdateMessage extends Header {
  int srcAddr @setter(setSrcAddr) @getter(getSrcAddr);
  int destAddr @setter(setDestAddr) @getter(getDestAddr);
}

packet InternalNeighborAddressesMessage extends Header {
  int srcAddr @setter(setSrcAddr) @getter(getSrcAddr);
  int destAddr @setter(setDestAddr) @getter(getDestAddr);
  unsigned long RuleSet_id;
  int stack_of_NeighboringQNodeIndices[];
}


