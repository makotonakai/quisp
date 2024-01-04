#include "JsonLogger.h"
#include <sstream>
#include "messages/QNode_ipc_messages_m.h"
#include "messages/barrier_messages_m.h"
#include "messages/connection_setup_messages_m.h"

namespace quisp::modules::Logger {

using quisp::messages::ConnectionSetupRequest;
using quisp::messages::ConnectionSetupResponse;
using quisp::messages::ConnectionTeardownMessage;
using quisp::messages::RejectConnectionSetupRequest;

JsonLogger::JsonLogger(std::shared_ptr<spdlog::logger> logger) : _logger(logger) {
  std::string jsonpattern = {"{%v}"};
  _logger->set_pattern(jsonpattern);
}

JsonLogger::~JsonLogger() {}

void JsonLogger::setModule(omnetpp::cModule const* const mod) {
  module = mod;
  module_path = mod->getFullPath();
}

void JsonLogger::setQNodeAddress(int addr) { qnode_address = addr; }

void JsonLogger::logPacket(const std::string& event_type, omnetpp::cMessage const* const msg) {
  auto current_time = omnetpp::simTime();
  _logger->info("\"simtime\": {}, \"event_type\": \"{}\", \"address\": \"{}\", {}", current_time, event_type, qnode_address, format(msg));
}

void JsonLogger::logQubitState(quisp::modules::QNIC_type qnic_type, int qnic_index, int qubit_index, bool is_busy, bool is_allocated) {
  auto current_time = omnetpp::simTime();
  _logger->info(
      "\"simtime\": {}, \"event_type\": \"QubitStateChange\", \"address\": \"{}\", \"qnic_type\": {}, \"qnic_index\": {}, \"qubit_index\": {}, \"busy\": {}, \"allocated\": {}",
      current_time, qnode_address, qnic_type, qnic_index, qubit_index, is_busy, is_allocated);
}

std::string JsonLogger::format(omnetpp::cMessage const* const msg) {
  if (auto* req = dynamic_cast<const quisp::messages::ConnectionSetupRequest*>(msg)) {
    std::stringstream os;
    os << "\"msg_type\": \"ConnectionSetupRequest\"";
    os << ", \"application_id\": " << req->getApplicationId();
    os << ", \"actual_dest_addr\": " << req->getActual_destAddr();
    os << ", \"actual_src_addr\": " << req->getActual_srcAddr();
    os << ", \"num_measure\": " << req->getNum_measure();
    os << ", \"num_required_bell_pairs\": " << req->getNumber_of_required_Bellpairs();
    return os.str();
  }
  if (auto* req = dynamic_cast<const quisp::messages::RejectConnectionSetupRequest*>(msg)) {
    std::stringstream os;
    os << "\"msg_type\": \"RejectConnectionSetupRequest\"";
    os << ", \"application_id\": " << req->getApplicationId();
    os << ", \"actual_dest_addr\": " << req->getActual_destAddr();
    os << ", \"actual_src_addr\": " << req->getActual_srcAddr();
    os << ", \"num_required_bell_pairs\": " << req->getNumber_of_required_Bellpairs();
    return os.str();
  }
  if (auto* req = dynamic_cast<const quisp::messages::ConnectionSetupResponse*>(msg)) {
    std::stringstream os;
    os << "\"msg_type\": \"ConnectionSetupResponse\"";
    os << ", \"application_id\": " << req->getApplicationId();
    os << ", \"actual_dest_addr\": " << req->getActual_destAddr();
    os << ", \"actual_src_addr\": " << req->getActual_srcAddr();
    os << ", \"ruleset_id\": " << req->getRuleSet_id();
    os << ", \"ruleset\": " << req->getRuleSet();
    os << ", \"application_type\": " << req->getApplication_type();
    os << ", \"stack_of_qnode_indices\": [";
    for (int i = 0; i < req->getStack_of_QNodeIndexesArraySize(); i++) {
      if (i != 0) os << ", ";
      os << req->getStack_of_QNodeIndexes(i);
    }
    os << "]";
    return os.str();
  }
  if (auto* req = dynamic_cast<const quisp::messages::ConnectionTeardownMessage*>(msg)) {
    std::stringstream os;
    os << "\"msg_type\": \"ConnectionTeardownMessage\"";
    os << ", \"left_node_addr\": " << req->getLeftNodeAddr();
    os << ", \"right_node_addr\": " << req->getRightNodeAddr();
    os << ", \"ruleset_id\": " << req->getRuleSet_id();
    return os.str();
  }

  if (auto* req = dynamic_cast<const quisp::messages::InternalNodeAddressesAlongPathForwarding*>(msg)) {
    std::stringstream os;
    os << "\"msg_type\": \"InternalNodeAddressesAlongPathForwarding\"";
    for (auto i = 0; i < req->getNode_addresses_along_pathArraySize(); i++) {
      os << ", \"node_address_along_path\": " << req->getNode_addresses_along_path(i);
    }
    os << ", \"ruleset_id\": " << req->getRuleSet_id();
    return os.str();
  }

  if (auto* req = dynamic_cast<const quisp::messages::InternalRuleSetForwarding_Application*>(msg)) {
    std::stringstream os;
    os << "\"msg_type\": \"InternalRuleSetForwarding_Application\"";
    os << ", \"rule_id\": " << req->getRule_id();
    os << ", \"ruleset_id\": " << req->getRuleSet_id();
    os << ", \"ruleset\": " << req->getRuleSet();
    os << ", \"application_type\": " << req->getApplication_type();
    return os.str();
  }

  if (auto* req = dynamic_cast<const quisp::messages::InternalRuleSetForwarding*>(msg)) {
    std::stringstream os;
    os << "\"msg_type\": \"InternalRuleSetForwarding\"";
    os << ", \"rule_id\": " << req->getRule_id();
    os << ", \"ruleset_id\": " << req->getRuleSet_id();
    os << ", \"ruleset\": " << req->getRuleSet();
    return os.str();
  }

  if (auto* req = dynamic_cast<const quisp::messages::InternalConnectionTeardownMessage*>(msg)) {
    std::stringstream os;
    os << "\"msg_type\": \"InternalConnectionTeardownMessage\"";
    os << ", \"left_node_addr\": " << req->getLeftNodeAddr();
    os << ", \"right_node_addr\": " << req->getRightNodeAddr();
    os << ", \"ruleset_id\": " << req->getRuleSet_id();
    return os.str();
  }

  if (auto* req = dynamic_cast<const quisp::messages::LinkAllocationUpdateNotifier*>(msg)) {
    std::stringstream os;
    os << "\"msg_type\": \"LinkAllocationUpdateNotifier\"";
    os << ", \"dest_addr\": " << req->getDestAddr();
    os << ", \"src_addr\": " << req->getSrcAddr();
    os << ", \"stack_of_qnode_indices\": [";
    for (int i = 0; i < req->getStack_of_NeighboringQNodeIndicesArraySize(); i++) {
      if (i != 0) os << ", ";
      os << req->getStack_of_NeighboringQNodeIndices(i);
    }
    os << "]";
    return os.str();
  }

  if (auto* req = dynamic_cast<const quisp::messages::BarrierMessage*>(msg)) {
    std::stringstream os;
    os << "\"msg_type\": \"BarrierMessage\"";
    os << ", \"dest_addr\": " << req->getDestAddr();
    os << ", \"src_addr\": " << req->getSrcAddr();
    os << ", \"sequence_number\": " << req->getSequenceNumber();
    return os.str();
  }

  if (auto* req = dynamic_cast<const quisp::messages::WaitMessage*>(msg)) {
    std::stringstream os;
    os << "\"msg_type\": \"WaitMessage\"";
    os << ", \"dest_addr\": " << req->getDestAddr();
    os << ", \"src_addr\": " << req->getSrcAddr();
    os << ", \"dest_addr_of_barrier_message\": " << req->getDestAddrOfBarrierMessage();
    return os.str();
  }

  if (auto* req = dynamic_cast<const quisp::messages::ConnectionTeardownNotifier*>(msg)) {
    std::stringstream os;
    os << "\"msg_type\": \"ConnectionTeardownNotifier\"";
    os << ", \"dest_addr\": " << req->getDestAddr();
    os << ", \"src_addr\": " << req->getSrcAddr();
    os << ", \"stack_of_ruleset_id\": [";
    for (int i = 0; i < req->getRuleSetIdCount(); i++) {
      if (i != 0) os << ", ";
      os << req->getRuleSetIds(i);
    }
    os << "]";
    return os.str();
  }

  if (auto* req = dynamic_cast<const quisp::messages::LinkAllocationUpdateMessage*>(msg)) {
    std::stringstream os;
    os << "\"msg_type\": \"LinkAllocationUpdateMessage\"";
    os << ", \"dest_addr\": " << req->getDestAddr();
    os << ", \"src_addr\": " << req->getSrcAddr();
    os << ", \"stack_of_active_link_allocations\": [";
    for (int i = 0; i < req->getActiveLinkAllocationCount(); i++) {
      if (i != 0) os << ", ";
      os << req->getActiveLinkAllocations(i);
    }
    os << "]";
    os << ", \"stack_of_next_link_allocations\": [";
    for (int i = 0; i < req->getNextLinkAllocationCount(); i++) {
      if (i != 0) os << ", ";
      os << req->getNextLinkAllocations(i);
    }
    os << "]";
    os << ", \"random_number\": " << req->getRandomNumber();
    return os.str();
  }

  return "\"msg\": \"unknown class\": \"" + msg->getFullPath() + "\"";
}

void JsonLogger::logBellPairInfo(const std::string& event_type, int sequence_number, int partner_addr, quisp::modules::QNIC_type qnic_type, int qnic_index, int qubit_index) {
  auto current_time = omnetpp::simTime();
  _logger->info(
      "\"simtime\": {}, \"event_type\": \"BellPair{}\", \"address\": \"{}\", \"sequence_number\": {}, \"partner_addr\": {}, \"qnic_type\": {}, \"qnic_index\": {}, "
      "\"qubit_index\": {}",
      current_time, event_type, qnode_address, sequence_number, partner_addr, qnic_type, qnic_index, qubit_index);
}

}  // namespace quisp::modules::Logger
