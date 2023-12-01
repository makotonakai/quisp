cplusplus  {{
    #include "base_messages_m.h"
    #include <modules/QRSA/RuleEngine/QubitRecord/QubitRecord.h>
    using quisp::modules::qubit_record::IQubitRecord;
}}

class IQubitRecord {
    @existingClass;
    @opaque;
};

import base_messages;
namespace quisp::messages;

packet BarrierRequest extends Header
{
    int actualSrcAddr;
    int actualDestAddr;
    unsigned long stack_of_ActiveLinkAllocations[];
    int sequenceNumber @setter(setSequenceNumber) @getter(getSequenceNumber);
}

packet RejectBarrierRequest extends Header
{

    int actualSrcAddr;
    int actualDestAddr;
    unsigned long stack_of_ActiveLinkAllocations[] @appender(appendActiveLinkAllocation) @getter(getActiveLinkAllocations) @sizeGetter(getActiveLinkAllocationCount);
    int sequenceNumber @setter(setSequenceNumber) @getter(getSequenceNumber);
}

packet BarrierResponse extends Header
{

    int actualSrcAddr;
    int actualDestAddr;
    unsigned long stack_of_ActiveLinkAllocations[] @appender(appendActiveLinkAllocation) @getter(getActiveLinkAllocations) @sizeGetter(getActiveLinkAllocationCount);
    int sequenceNumber @setter(setSequenceNumber) @getter(getSequenceNumber);
}

packet WaitMessage extends Header {
    int actualDestAddr;
    unsigned long stack_of_ActiveLinkAllocations[] @appender(appendActiveLinkAllocation) @getter(getActiveLinkAllocations) @sizeGetter(getActiveLinkAllocationCount);
}
