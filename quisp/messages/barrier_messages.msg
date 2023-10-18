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
    unsigned long negotiatedRulesetId @setter(setNegotiatedRulesetId) @getter(getNegotiatedRuleSetId);
    IQubitRecord *qubitRecord @setter(setQubitRecord) @getter(getQubitRecord);
    int sequenceNumber; 
    bool isSender;   
    bool isLast;
}
