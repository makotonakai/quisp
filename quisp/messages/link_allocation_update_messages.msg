import base_messages;

namespace quisp::messages;

packet LinkAllocationUpdateMessage extends Header {
  int srcAddr @setter(setSrcAddr) @getter(getSrcAddr);
  int destAddr @setter(setDestAddr) @getter(getDestAddr);
  bool is_sender @setter(setIsSender) @getter(getIsSender);
  unsigned long stack_of_ActiveLinkAllocations[];
  int random_number @setter(setRandomNumber) @getter(getRandomNumber); 
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


