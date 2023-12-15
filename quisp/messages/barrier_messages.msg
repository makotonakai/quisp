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
    int sequenceNumber @setter(setSequenceNumber) @getter(getSequenceNumber);
}

packet WaitMessage extends Header
{
    int destAddrOfBarrierMessage @setter(setDestAddrOfBarrierMessage) @getter(getDestAddrOfBarrierMessage);
}



