import base_messages;
namespace quisp::messages;



packet ConnectionTeardownMessage extends Header
{
    int left_nodeAddr @getter(getLeftNodeAddr) @setter(setLeftNodeAddr);
    int right_nodeAddr @getter(getRightNodeAddr) @setter(setRightNodeAddr);
    unsigned long RuleSet_id;
}

packet InternalConnectionTeardownMessage extends Header
{
    int left_nodeAddr @getter(getLeftNodeAddr) @setter(setLeftNodeAddr);
    int right_nodeAddr @getter(getRightNodeAddr) @setter(setRightNodeAddr);
    unsigned long RuleSet_id;
    int stack_of_QNICAddresses[];
}

packet ConnectionTeardownNotifier extends Header {
    unsigned long rulesetIds[] @appender(appendRuleSetId) @getter(getRuleSetIds) @sizeGetter(getRuleSetIdCount);
}

