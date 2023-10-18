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

packet BarrierMessage extends Header
{
    int srcAddr;
    int destAddr;
    int actualSrcAddr;
    int actualDestAddr;
    unsigned long rulesetId @setter(setRulesetId) @getter(getRuleSetId);
    int sequenceNumber @setter(setSequenceNumber) @getter(getSequenceNumber); 
    bool isSender @setter(setIsSender) @getter(getIsSender);   
}
