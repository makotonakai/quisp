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
    int srcAddr;
    int destAddr;
    int actualSrcAddr;
    int actualDestAddr;
    unsigned long rulesetId @setter(setRuleSetId) @getter(getRuleSetId);
    int sequenceNumber @setter(setSequenceNumber) @getter(getSequenceNumber); 
}

packet RejectBarrierRequest extends Header
{
    int srcAddr;
    int destAddr;
    int actualSrcAddr;
    int actualDestAddr;
    unsigned long rulesetId @setter(setRuleSetId) @getter(getRuleSetId);
    int sequenceNumber @setter(setSequenceNumber) @getter(getSequenceNumber); 
}

packet BarrierResponse extends Header
{
    int srcAddr;
    int destAddr;
    int actualSrcAddr;
    int actualDestAddr;
    unsigned long rulesetId @setter(setRuleSetId) @getter(getRuleSetId);
    int sequenceNumber @setter(setSequenceNumber) @getter(getSequenceNumber); 
}
