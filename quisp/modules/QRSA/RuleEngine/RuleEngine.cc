/** \file RuleEngine.cc
 *
 *  \brief RuleEngine
 */
#include "RuleEngine.h"
#include <unistd.h>
#include <cassert>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "BellPairStore/BellPairStore.h"
#include "QNicStore/QNicStore.h"
#include "RuntimeCallback.h"
#include "messages/QNode_ipc_messages_m.h"
#include "messages/barrier_messages_m.h"
#include "messages/connection_teardown_messages_m.h"
#include "messages/link_allocation_update_messages_m.h"
#include "messages/tomography_messages_m.h"
#include "modules/PhysicalConnection/BSA/types.h"
#include "modules/QRSA/RuleEngine/QubitRecord/IQubitRecord.h"
#include "rules/RuleSet.h"
#include "runtime/RuleSet.h"
#include "runtime/Runtime.h"

using quisp::runtime::Runtime;
#include "messages/BSA_ipc_messages_m.h"
#include "messages/QNode_ipc_messages_m.h"
#include "messages/link_generation_messages_m.h"
#include "modules/PhysicalConnection/BSA/types.h"
#include "modules/QNIC.h"
#include "omnetpp/csimulation.h"
#include "omnetpp/errmsg.h"
#include "omnetpp/simtime_t.h"

namespace quisp::modules {

using namespace std;
using namespace rules;
using namespace messages;
using qnic_store::QNicStore;
using runtime_callback::RuntimeCallback;

RuleEngine::RuleEngine() : provider(utils::ComponentProvider{this}), runtimes(std::make_unique<RuntimeCallback>(this)) {}

RuleEngine::~RuleEngine() {
  for (int i = 0; i < number_of_qnics; i++) cancelAndDelete(emit_photon_timer_map[{QNIC_type::QNIC_E, i}]);
  for (int i = 0; i < number_of_qnics_r; i++) cancelAndDelete(emit_photon_timer_map[{QNIC_type::QNIC_R, i}]);
  for (int i = 0; i < number_of_qnics_rp; i++) cancelAndDelete(emit_photon_timer_map[{QNIC_type::QNIC_RP, i}]);
}

void RuleEngine::initialize() {
  // HardwareMonitor's neighbor table is checked in the initialization stage of the simulation
  // This assumes the topology never changes throughout the simulation.
  // If dynamic change in topology is required, recoding this is needed.
  hardware_monitor = provider.getHardwareMonitor();
  realtime_controller = provider.getRealTimeController();
  routingdaemon = provider.getRoutingDaemon();
  initializeLogger(provider);
  bell_pair_store.logger = logger;

  parentAddress = provider.getNodeAddr();
  qnode_indices = {};

  number_of_qnics_all = par("total_number_of_qnics");
  number_of_qnics = par("number_of_qnics");
  number_of_qnics_r = par("number_of_qnics_r");
  number_of_qnics_rp = par("number_of_qnics_rp");
  if (qnic_store == nullptr) {
    qnic_store = std::make_unique<QNicStore>(provider, number_of_qnics, number_of_qnics_r, number_of_qnics_rp, logger);
  }
  for (int i = 0; i < number_of_qnics; i++) {
    emit_photon_timer_map[{QNIC_type::QNIC_E, i}] = new EmitPhotonRequest();
    emit_photon_timer_map[{QNIC_type::QNIC_E, i}]->setQnicType(QNIC_type::QNIC_E);
    emit_photon_timer_map[{QNIC_type::QNIC_E, i}]->setQnicIndex(i);
  }
  for (int i = 0; i < number_of_qnics_r; i++) {
    emit_photon_timer_map[{QNIC_type::QNIC_R, i}] = new EmitPhotonRequest();
    emit_photon_timer_map[{QNIC_type::QNIC_R, i}]->setQnicType(QNIC_type::QNIC_R);
    emit_photon_timer_map[{QNIC_type::QNIC_R, i}]->setQnicIndex(i);
  }
  for (int i = 0; i < number_of_qnics_rp; i++) {
    emit_photon_timer_map[{QNIC_type::QNIC_RP, i}] = new EmitPhotonRequest();
    emit_photon_timer_map[{QNIC_type::QNIC_RP, i}]->setQnicType(QNIC_type::QNIC_RP);
    emit_photon_timer_map[{QNIC_type::QNIC_RP, i}]->setQnicIndex(i);
  }
}

void RuleEngine::handleMessage(cMessage *msg) {
  logger->logPacket("handleRuleEngineMessage", msg);

  if (auto *notification_packet = dynamic_cast<BSMTimingNotification *>(msg)) {
    if (auto *bsa_results = dynamic_cast<CombinedBSAresults *>(msg)) {
      handleLinkGenerationResult(bsa_results);
    }
    auto type = notification_packet->getQnicType();
    auto qnic_index = notification_packet->getQnicIndex();
    stopOnGoingPhotonEmission(type, qnic_index);
    freeFailedEntanglementAttemptQubits(type, qnic_index);
    schedulePhotonEmission(type, qnic_index, notification_packet);
  } else if (auto *pk = dynamic_cast<EmitPhotonRequest *>(msg)) {
    auto type = pk->getQnicType();
    auto qnic_index = pk->getQnicIndex();
    auto number_of_free_emitters = qnic_store->countNumFreeQubits(type, qnic_index);
    auto qubit_index = qnic_store->takeFreeQubitIndex(type, qnic_index);
    // If this is MSM, we keep on emmiting photons continuously
    if (pk->isMSM()) {
      auto &msm_info = msm_info_map[qnic_index];
      msm_info.photon_index_counter++;
      if (number_of_free_emitters != 0) {
        msm_info.qubit_info_map[msm_info.iteration_index] = qubit_index;
        sendEmitPhotonSignalToQnic(type, qnic_index, qubit_index, true, true);
      } else {
        // send MSMResult to partner node, even if we fail to have BSM happen
        MSMResult *msm_result = new MSMResult();
        msm_result->setQnicIndex(msm_info.partner_qnic_index);
        msm_result->setQnicType(QNIC_RP);
        msm_result->setPhotonIndex(msm_info.photon_index_counter);
        msm_result->setSuccess(false);
        msm_result->setCorrectionOperation(PauliOperator ::I);
        msm_result->setSrcAddr(parentAddress);
        msm_result->setDestAddr(msm_info.partner_address);
        msm_result->setKind(6);
        send(msm_result, "RouterPort$o");
      }
      scheduleAt(simTime() + pk->getIntervalBetweenPhotons(), pk);
      return;
      // If not, we emit photons on demand
    } else {
      if (number_of_free_emitters == 0) return;
      auto is_first = pk->isFirst();
      auto is_last = (number_of_free_emitters == 1);
      // need to set is_first to false
      pk->setFirst(false);
      sendEmitPhotonSignalToQnic(type, qnic_index, qubit_index, is_first, is_last);
      if (!is_last) {
        scheduleAt(simTime() + pk->getIntervalBetweenPhotons(), pk);
      }
      // early return since this doesn't affect entangled resource
      // and we don't want to delete these messages
      return;
    }
    // store message from epps to rule engine
  } else if (auto *notification_packet = dynamic_cast<EPPSTimingNotification *>(msg)) {
    auto partner_address = notification_packet->getOtherQnicParentAddr();
    auto partner_qnic_index = notification_packet->getOtherQnicIndex();
    auto epps_address = notification_packet->getEPPSAddr();
    auto qnic_index = notification_packet->getQnicIndex();
    auto &msm_info = msm_info_map[qnic_index];
    msm_info.partner_address = partner_address;
    msm_info.epps_address = epps_address;
    msm_info.partner_qnic_index = partner_qnic_index;
    msm_info.total_travel_time = notification_packet->getTotalTravelTime();
    stopOnGoingPhotonEmission(QNIC_RP, qnic_index);
    scheduleMSMPhotonEmission(QNIC_RP, qnic_index, notification_packet);
    // handle result incoming from bsa
  } else if (auto *click_result = dynamic_cast<SingleClickResult *>(msg)) {
    handleSingleClickResult(click_result);
    // handle result incoming from partner node
  } else if (auto *msm_result = dynamic_cast<MSMResult *>(msg)) {
    handleMSMResult(msm_result);
  } else if (auto *pkt = dynamic_cast<StopEmitting *>(msg)) {
    handleStopEmitting(pkt);
  } else if (auto *pk = dynamic_cast<LinkTomographyRuleSet *>(msg)) {
    auto *ruleset = pk->getRuleSet();
    runtimes.acceptRuleSet(ruleset->construct());
  } else if (auto *pkt = dynamic_cast<PurificationResult *>(msg)) {
    handlePurificationResult(pkt);
  } else if (auto *pkt = dynamic_cast<SwappingResult *>(msg)) {
    handleSwappingResult(pkt);
    // handle self message to check whether partner's result has arrived
  } else if (auto *pkt = dynamic_cast<InternalRuleSetForwarding *>(msg)) {
    // add actual process
    auto serialized_ruleset = pkt->getRuleSet();
    RuleSet ruleset(0, 0);
    ruleset.deserialize_json(serialized_ruleset);
    runtimes.acceptRuleSet(ruleset.construct());

    sendLinkAllocationUpdateMessages();
    for (auto it = runtimes.begin(); it != runtimes.end(); ++it) {
      for (auto partner_addr : it->partners) {
        auto lau_sent = true;
        auto lau_received = false;
        if (node_address_lau_received_map.find(partner_addr.val) != node_address_lau_received_map.end()) {
          lau_received = node_address_lau_received_map[partner_addr.val];
        }
        if (lau_sent && lau_received) {
          negotiateNextLinkAllocationPolicy(partner_addr.val);
          auto bell_pair_exist = bellPairExist();
          auto barrier_sent = node_address_barrier_sent_map[partner_addr.val];
          if (bell_pair_exist && !barrier_sent) {
            sendBarrierMessage(partner_addr.val);
          } else {
            waitForBellPairGeneration(partner_addr.val);
          }
        }
      }
    }
  } else if (auto *pkt = dynamic_cast<InternalRuleSetForwarding_Application *>(msg)) {
    if (pkt->getApplication_type() != 0) error("This application is not recognized yet");
    auto serialized_ruleset = pkt->getRuleSet();
    RuleSet ruleset(0, 0);
    ruleset.deserialize_json(serialized_ruleset);
    runtimes.acceptRuleSet(ruleset.construct());

    sendLinkAllocationUpdateMessages();
    for (auto it = runtimes.begin(); it != runtimes.end(); ++it) {
      for (auto partner_addr : it->partners) {
        auto lau_sent = true;
        auto lau_received = false;
        if (node_address_lau_received_map.find(partner_addr.val) != node_address_lau_received_map.end()) {
          lau_received = node_address_lau_received_map[partner_addr.val];
        }
        if (lau_sent && lau_received) {
          negotiateNextLinkAllocationPolicy(partner_addr.val);
          auto bell_pair_exist = bellPairExist();
          auto barrier_sent = node_address_barrier_sent_map[partner_addr.val];
          if (bell_pair_exist && !barrier_sent) {
            sendBarrierMessage(partner_addr.val);
          } else {
            waitForBellPairGeneration(partner_addr.val);
          }
        }
      }
    }
  } else if (auto *pkt = dynamic_cast<InternalConnectionTeardownMessage *>(msg)) {
    handleInternalConnectionTeardownMessage(pkt);
  } else if (auto *pkt = dynamic_cast<LinkAllocationUpdateMessage *>(msg)) {
    storeInfoAboutIncomingLinkAllocationUpdateMessage(pkt);
    auto src_addr = pkt->getSrcAddr();
    auto lau_sent = node_address_lau_sent_map[src_addr];
    auto lau_received = node_address_lau_received_map[src_addr];
    if (lau_sent && lau_received) {
      auto next_link_allocation_policy_size = pkt->getNextLinkAllocationCount();
      if (next_link_allocation_policy_size != 0) {
        negotiateNextLinkAllocationPolicy(src_addr);
        auto src_addr = pkt->getSrcAddr();
        auto bell_pair_exist = bellPairExist();
        auto barrier_sent = node_address_barrier_sent_map[src_addr];
        if (bell_pair_exist && !barrier_sent) {
          sendBarrierMessage(src_addr);
        } else {
          waitForBellPairGeneration(src_addr);
        }
      }
    }
  } else if (auto *pkt = dynamic_cast<WaitMessage *>(msg)) {
    auto dest_addr = pkt->getDestAddrOfBarrierMessage();
    auto bell_pair_exist = bellPairExist();
    auto barrier_sent = node_address_barrier_sent_map[dest_addr];
    if (bell_pair_exist && !barrier_sent) {
      sendBarrierMessage(dest_addr);
    } else {
      keepWaitingForBellPairGeneration(pkt);
    }
  } else if (auto *pkt = dynamic_cast<BarrierMessage *>(msg)) {
    storeInfoAboutBarrierMessage(pkt);
    auto src_addr = pkt->getSrcAddr();
    auto barrier_sent = node_address_barrier_sent_map[src_addr];
    auto barrier_received = node_address_barrier_received_map[src_addr];
    if (barrier_sent && barrier_received) {
      negotiateNextSequenceNumber(src_addr);
    }
  }

  for (auto it = runtimes.begin(); it != runtimes.end(); ++it) {
    for (auto partner_address : it->partners) {
      if (node_address_sequence_number_map.find(partner_address.val) != node_address_sequence_number_map.end()) {
        auto sequence_number = node_address_sequence_number_map[partner_address.val];
        for (int i = 0; i < number_of_qnics; i++) {
          allocateBellPairs(QNIC_E, i, sequence_number);
        }
        for (int i = 0; i < number_of_qnics_r; i++) {
          allocateBellPairs(QNIC_R, i, sequence_number);
        }
        for (int i = 0; i < number_of_qnics_rp; i++) {
          allocateBellPairs(QNIC_RP, i, sequence_number);
        }
      }
    }
  }
  executeAllRuleSets();

  delete msg;
}

void RuleEngine::schedulePhotonEmission(QNIC_type type, int qnic_index, BSMTimingNotification *notification) {
  auto first_photon_emit_time = getEmitTimeFromBSMNotification(notification);
  auto *timer = emit_photon_timer_map[{type, qnic_index}];
  timer->setFirst(true);
  timer->setIntervalBetweenPhotons(notification->getInterval());
  timer->setMSM(false);
  scheduleAt(first_photon_emit_time, timer);
}

void RuleEngine::scheduleMSMPhotonEmission(QNIC_type type, int qnic_index, EPPSTimingNotification *notification) {
  auto first_photon_emit_time = notification->getFirstPhotonEmitTime();
  auto *timer = emit_photon_timer_map[{type, qnic_index}];
  timer->setFirst(true);
  timer->setIntervalBetweenPhotons(notification->getInterval());
  timer->setMSM(true);
  scheduleAt(first_photon_emit_time, timer);
}

void RuleEngine::sendEmitPhotonSignalToQnic(QNIC_type qnic_type, int qnic_index, int qubit_index, bool is_first, bool is_last) {
  int pulse = 0;
  if (is_first) pulse |= STATIONARYQUBIT_PULSE_BEGIN;
  if (is_last) pulse |= STATIONARYQUBIT_PULSE_END;
  realtime_controller->EmitPhoton(qnic_index, qubit_index, qnic_type, pulse);
  if (qnic_type != QNIC_RP) emitted_photon_order_map[{qnic_type, qnic_index}].push_back(qubit_index);
}
simtime_t RuleEngine::getEmitTimeFromBSMNotification(quisp::messages::BSMTimingNotification *notification) { return notification->getFirstPhotonEmitTime(); }

void RuleEngine::stopOnGoingPhotonEmission(QNIC_type type, int qnic_index) { cancelEvent(emit_photon_timer_map[{type, qnic_index}]); }

void RuleEngine::freeFailedEntanglementAttemptQubits(QNIC_type type, int qnic_index) {
  auto &emitted_indices = emitted_photon_order_map[{type, qnic_index}];
  for (auto qubit_index : emitted_indices) {
    realtime_controller->ReInitialize_StationaryQubit(qnic_index, qubit_index, type, false);
    qnic_store->setQubitBusy(type, qnic_index, qubit_index, false);
  }
  emitted_indices.clear();
}

void RuleEngine::handleSingleClickResult(SingleClickResult *click_result) {
  auto qnic_index = click_result->getQnicIndex();
  auto &msm_info = msm_info_map[qnic_index];
  auto qubit_index = msm_info.qubit_info_map[msm_info.iteration_index];
  MSMResult *msm_result = new MSMResult();
  msm_result->setQnicIndex(msm_info.partner_qnic_index);
  msm_result->setQnicType(QNIC_RP);
  msm_result->setPhotonIndex(msm_info.photon_index_counter);
  msm_result->setSuccess(click_result->getClickResult().success);
  msm_result->setCorrectionOperation(click_result->getClickResult().correction_operation);
  msm_result->setSrcAddr(parentAddress);
  msm_result->setDestAddr(msm_info.partner_address);
  msm_result->setKind(6);
  if (click_result->getClickResult().success) {
    msm_info.qubit_postprocess_info[msm_info.photon_index_counter].qubit_index = qubit_index;
    msm_info.qubit_postprocess_info[msm_info.photon_index_counter].correction_operation = click_result->getClickResult().correction_operation;
    msm_info.iteration_index++;
  } else {
    realtime_controller->ReInitialize_StationaryQubit(qnic_index, qubit_index, QNIC_RP, false);
    qnic_store->setQubitBusy(QNIC_RP, qnic_index, qubit_index, false);
  }
  send(msm_result, "RouterPort$o");
}

void RuleEngine::handleMSMResult(MSMResult *msm_result) {
  auto qnic_index = msm_result->getQnicIndex();
  auto &msm_info = msm_info_map[qnic_index];
  auto qubit_itr = msm_info.qubit_postprocess_info.find(msm_result->getPhotonIndex());
  // local: fail | partner: success/fail
  // qubit on photon index is not included in msm_info
  if (qubit_itr == msm_info.qubit_postprocess_info.end()) {
    return;
  }
  QubitInfo qubit_info = qubit_itr->second;
  auto qubit_index = qubit_info.qubit_index;
  // local: success | partner: fail
  // qubit on photon index is included in msm_info but the partner sends fail
  if (!msm_result->getSuccess()) {
    realtime_controller->ReInitialize_StationaryQubit(qnic_index, qubit_index, QNIC_RP, false);
    qnic_store->setQubitBusy(QNIC_RP, qnic_index, qubit_index, false);
  }
  // local: success | partner: success
  // qubit on photon index is included in msm_info and the partner sends success
  else {
    auto *qubit_record = qnic_store->getQubitRecord(QNIC_RP, qnic_index, qubit_index);
    // condition whether to apply Z gate or not
    bool is_phi_minus = qubit_info.correction_operation != msm_result->getCorrectionOperation();
    // restrict correction operation only on one side
    bool is_younger_address = parentAddress < msm_info.partner_address;
    if (is_phi_minus && is_younger_address) realtime_controller->applyZGate(qubit_record);
    bell_pair_store.insertEntangledQubit(msm_info.photon_index_counter, msm_info.partner_address, qubit_record);
  }
}

void RuleEngine::handleLinkGenerationResult(CombinedBSAresults *bsa_result) {
  auto type = bsa_result->getQnicType();
  auto qnic_index = bsa_result->getQnicIndex();
  auto num_success = bsa_result->getSuccessCount();
  auto partner_address = bsa_result->getNeighborAddress();
  auto &emitted_indices = emitted_photon_order_map[{type, qnic_index}];
  for (int i = num_success - 1; i >= 0; i--) {
    auto emitted_index = bsa_result->getSuccessfulPhotonIndices(i);
    auto qubit_index = emitted_indices[emitted_index];
    auto *qubit_record = qnic_store->getQubitRecord(type, qnic_index, qubit_index);
    auto iterator = emitted_indices.begin();
    std::advance(iterator, emitted_index);
    auto sequence_number = bsa_result->getSequenceNumbers(num_success - 1 - i);
    bell_pair_store.insertEntangledQubit(sequence_number, partner_address, qubit_record);
    emitted_indices.erase(iterator);

    auto correction_operation = bsa_result->getCorrectionOperationList(i);
    if (correction_operation == PauliOperator::X) {
      realtime_controller->applyXGate(qubit_record);
    } else if (correction_operation == PauliOperator::Z) {
      realtime_controller->applyZGate(qubit_record);
    } else if (correction_operation == PauliOperator::Y) {
      realtime_controller->applyYGate(qubit_record);
    }
  }
}

void RuleEngine::handleStopEmitting(StopEmitting *stop_emit) {
  int qnic_index = stop_emit->getQnic_address();
  auto &msm_info = msm_info_map[qnic_index];
  // only do the following procedure for MSM links
  if (msm_info.photon_index_counter == 0) return;
  StopEPPSEmission *stop_epps_emission = new StopEPPSEmission();
  stop_epps_emission->setSrcAddr(parentAddress);
  stop_epps_emission->setDestAddr(msm_info.epps_address);
  send(stop_epps_emission, "RouterPort$o");
}

void RuleEngine::handlePurificationResult(PurificationResult *result) {
  auto ruleset_id = result->getRulesetId();
  auto shared_rule_tag = result->getSharedRuleTag();
  auto sequence_number = result->getSequenceNumber();
  auto measurement_result = result->getMeasurementResult();
  auto purification_protocol = result->getProtocol();
  std::vector<int> message_content = {sequence_number, measurement_result, purification_protocol};
  auto runtime = runtimes.findById(ruleset_id);
  if (runtime == runtimes.end()) return;
  runtime->assignMessageToRuleSet(shared_rule_tag, message_content);
}

void RuleEngine::handleSwappingResult(SwappingResult *result) {
  auto ruleset_id = result->getRulesetId();
  auto shared_rule_tag = result->getSharedRuleTag();
  auto sequence_number = result->getSequenceNumber();
  auto correction_frame = result->getCorrectionFrame();
  auto new_partner_addr = result->getNewPartner();
  std::vector<int> message_content = {sequence_number, correction_frame, new_partner_addr};
  auto runtime = runtimes.findById(ruleset_id);
  if (runtime == runtimes.end()) return;
  runtime->assignMessageToRuleSet(shared_rule_tag, message_content);
}

void RuleEngine::handleInternalConnectionTeardownMessage(InternalConnectionTeardownMessage *msg) {
  stopRuleSetExecution(msg);
  auto terminated_runtimes = runtimes.getTerminatedRuntimes();
  for (auto it = terminated_runtimes.begin(); it != terminated_runtimes.end(); ++it) {
    for (auto partner_address : it->partners) {
      if (node_address_sequence_number_map.find(partner_address.val) != node_address_sequence_number_map.end()) {
        auto sequence_number = node_address_sequence_number_map[partner_address.val];
        for (int i = 0; i < number_of_qnics; i++) {
          releaseBellPairs(QNIC_E, i, sequence_number);
        }
        for (int i = 0; i < number_of_qnics_r; i++) {
          releaseBellPairs(QNIC_R, i, sequence_number);
        }
        for (int i = 0; i < number_of_qnics_rp; i++) {
          releaseBellPairs(QNIC_RP, i, sequence_number);
        }
      }
    }
    auto ruleset_id = it->ruleset.id;
    removeRuleSetIdFromActiveLinkAllocationPolicy(ruleset_id);
  }
  sendLinkAllocationUpdateMessages();
  partner_addresses.clear();
}

void RuleEngine::stopRuleSetExecution(InternalConnectionTeardownMessage *msg) {
  auto ruleset_id = msg->getRuleSetId();
  runtimes.stopById(ruleset_id);
}

void RuleEngine::removeRuleSetIdFromActiveLinkAllocationPolicy(unsigned long ruleset_id) {
  auto terminated_runtimes = runtimes.getTerminatedRuntimes();
  for (auto it = terminated_runtimes.begin(); it != terminated_runtimes.end(); ++it) {
    for (auto partner_address : it->partners) {
      for (auto it2 = node_address_active_link_allocations_map[partner_address.val].begin(); it2 != node_address_active_link_allocations_map[partner_address.val].end();) {
        if (*it2 == ruleset_id) {
          node_address_active_link_allocations_map[partner_address.val].erase(it2);
        } else {
          ++it2;
        }
      }
    }
  }
}

void RuleEngine::sendLinkAllocationUpdateMessages() {
  if (partner_addresses.size() == 0) {
    for (auto it = runtimes.begin(); it != runtimes.end(); ++it) {
      for (auto partner_address : it->partners) {
        partner_addresses.insert(partner_address.val);
      }
    }
  }

  for (auto it = runtimes.begin(); it != runtimes.end(); ++it) {
    for (auto partner_address : it->partners) {
      std::vector<int> active_link_allocations;
      if (node_address_active_link_allocations_map.find(partner_address.val) != node_address_active_link_allocations_map.end()) {
        for (auto active_link_allocation : node_address_active_link_allocations_map[partner_address.val]) {
          active_link_allocations.push_back(active_link_allocation);
        }
      }

      std::vector<int> next_link_allocations;
      if (node_address_next_link_allocations_map.find(partner_address.val) != node_address_next_link_allocations_map.end()) {
        for (auto next_link_allocation : node_address_next_link_allocations_map[partner_address.val]) {
          next_link_allocations.push_back(next_link_allocation);
        }
      }

      if (it->is_active && std::find(active_link_allocations.begin(), active_link_allocations.end(), it->ruleset.id) == active_link_allocations.end()) {
        node_address_active_link_allocations_map[partner_address.val].push_back(it->ruleset.id);
      }
      if (!it->is_active && std::find(next_link_allocations.begin(), next_link_allocations.end(), it->ruleset.id) == next_link_allocations.end()) {
        node_address_next_link_allocations_map[partner_address.val].push_back(it->ruleset.id);
      }
    }
  }

  for (auto partner_address : partner_addresses) {
    LinkAllocationUpdateMessage *pkt = new LinkAllocationUpdateMessage("LinkAllocationUpdateMessage");
    pkt->setSrcAddr(parentAddress);
    pkt->setDestAddr(partner_address);

    auto active_link_allocations = node_address_active_link_allocations_map[partner_address];
    for (auto active_link_allocation : active_link_allocations) {
      pkt->appendActiveLinkAllocation(active_link_allocation);
    }

    auto next_link_allocations = node_address_next_link_allocations_map[partner_address];
    for (auto next_link_allocation : next_link_allocations) {
      pkt->appendNextLinkAllocation(next_link_allocation);
    }

    auto random_number = rand();
    node_address_random_number_map[partner_address] = random_number;

    pkt->setRandomNumber(random_number);
    send(pkt, "RouterPort$o");

    node_address_lau_sent_map[partner_address] = true;
  }
}

void RuleEngine::storeInfoAboutIncomingLinkAllocationUpdateMessage(LinkAllocationUpdateMessage *msg) {
  auto src_addr = msg->getSrcAddr();
  node_address_incoming_random_number_map[src_addr] = msg->getRandomNumber();

  auto incoming_active_link_allocations_count = msg->getActiveLinkAllocationCount();
  for (auto i = 0; i < incoming_active_link_allocations_count; i++) {
    auto incoming_active_link_allocation = msg->getActiveLinkAllocations(i);
    node_address_incoming_active_link_allocations_map[src_addr].push_back(incoming_active_link_allocation);
  }

  auto incoming_next_link_allocations_count = msg->getNextLinkAllocationCount();
  for (auto i = 0; i < incoming_next_link_allocations_count; i++) {
    auto incoming_next_link_allocation = msg->getNextLinkAllocations(i);
    node_address_incoming_next_link_allocations_map[src_addr].push_back(incoming_next_link_allocation);
  }
  node_address_lau_received_map[src_addr] = true;
}

void RuleEngine::negotiateNextLinkAllocationPolicy(int src_addr) {
  auto incoming_random_number = node_address_incoming_random_number_map[src_addr];
  auto random_number = node_address_random_number_map[src_addr];
  if (incoming_random_number > random_number) {
    node_address_next_link_allocations_map[src_addr].clear();
    for (auto it = node_address_incoming_next_link_allocations_map[src_addr].begin(); it != node_address_incoming_next_link_allocations_map[src_addr].end(); ++it) {
      node_address_next_link_allocations_map[src_addr].push_back(*it);
    }
  }
  node_address_barrier_sent_map[src_addr] = false;
  node_address_barrier_received_map[src_addr] = false;
}

void RuleEngine::sendBarrierMessage(int src_addr) {
  BarrierMessage *pkt = new BarrierMessage("BarrierMessage");
  pkt->setSrcAddr(parentAddress);
  pkt->setDestAddr(src_addr);
  auto sequence_number = bell_pair_store.getFirstAvailableSequenceNumber();
  pkt->setSequenceNumber(sequence_number);
  send(pkt, "RouterPort$o");

  node_address_barrier_sent_map[src_addr] = true;
  node_address_sequence_number_map[src_addr] = sequence_number;
}

void RuleEngine::storeInfoAboutBarrierMessage(BarrierMessage *msg) {
  auto src_addr = msg->getSrcAddr();
  node_address_barrier_received_map[src_addr] = true;
  auto sequence_number = msg->getSequenceNumber();
  node_address_incoming_sequence_number_map[src_addr] = sequence_number;
}

void RuleEngine::negotiateNextSequenceNumber(int src_addr) {
  auto incoming_sequence_number = node_address_incoming_sequence_number_map[src_addr];
  auto sequence_number = node_address_sequence_number_map[src_addr];
  node_address_sequence_number_map[src_addr] = std::max(incoming_sequence_number, sequence_number);
}

void RuleEngine::waitForBellPairGeneration(int src_addr) {
  WaitMessage *pkt = new WaitMessage("WaitMessage");
  pkt->setSrcAddr(parentAddress);
  pkt->setDestAddr(parentAddress);
  pkt->setDestAddrOfBarrierMessage(src_addr);
  scheduleAt(simTime() + 0.01, pkt);
}

void RuleEngine::keepWaitingForBellPairGeneration(WaitMessage *msg) {
  WaitMessage *pkt = new WaitMessage("WaitMessage");
  pkt->setSrcAddr(parentAddress);
  pkt->setDestAddr(parentAddress);
  pkt->setDestAddrOfBarrierMessage(msg->getDestAddrOfBarrierMessage());
  scheduleAt(simTime() + 0.01, pkt);
}

bool RuleEngine::bellPairExist() {
  auto sequence_number = bell_pair_store.getFirstAvailableSequenceNumber();
  return sequence_number != -1;
}

// Invoked whenever a new resource (entangled with neighbor) has been created.
// Allocates those resources to a particular ruleset, from top to bottom (all of it).
void RuleEngine::allocateBellPairs(int qnic_type, int qnic_index, int first_sequence_number) {
  std::map<int, std::vector<int>> partner_addr_runtime_indices_map;
  auto index = 0;
  for (auto it = runtimes.begin(); it != runtimes.end(); ++it) {
    auto partners = it->partners;
    for (auto partner : partners) {
      partner_addr_runtime_indices_map[partner.val].push_back(index);
    }
    index += 1;
  }

  for (const auto &[partner_addr, _] : partner_addr_runtime_indices_map) {
    auto runtime_indices = partner_addr_runtime_indices_map[partner_addr];
    auto bell_pair_range = bell_pair_store.getBellPairsRange((QNIC_type)qnic_type, qnic_index, partner_addr);

    auto bell_pair_num = 0;
    for (auto it = bell_pair_range.first; it != bell_pair_range.second; ++it) {
      bell_pair_num += 1;
    }

    auto number = 0;
    for (auto it = bell_pair_range.first; it != bell_pair_range.second; ++it) {
      auto sequence_number_qubit_record = it->second;
      auto sequence_number = sequence_number_qubit_record.first;
      if (first_sequence_number <= sequence_number) {
        auto qubit_record = sequence_number_qubit_record.second;
        if (!qubit_record->isAllocated()) {
          qubit_record->setAllocated(true);
          auto index = number * runtime_indices.size() / bell_pair_num;
          auto runtime_index = runtime_indices[index];
          runtimes.at(runtime_index).assignQubitToRuleSet(partner_addr, qubit_record);
          sequence_number_ruleset_id_map[sequence_number] = runtimes.at(runtime_index).ruleset.id;
          if (runtime_index_bell_pair_number_map.find(runtime_index) == runtime_index_bell_pair_number_map.end()) {
            runtime_index_bell_pair_number_map[runtime_index] = 0;
          } else {
            runtime_index_bell_pair_number_map[runtime_index] = runtime_index_bell_pair_number_map[runtime_index] + 1;
          }
          sequence_number_runtime_index_map[sequence_number] = runtime_index;
        }
      }
      number += 1;
    }
  }
}

void RuleEngine::releaseBellPairs(int qnic_type, int qnic_index, int first_sequence_number) {
  std::map<int, std::vector<int>> partner_addr_terminated_runtime_indices_map;
  auto index = 0;
  auto terminated_runtimes = runtimes.getTerminatedRuntimes();
  for (auto it = terminated_runtimes.begin(); it != terminated_runtimes.end(); ++it) {
    auto partners = it->partners;
    for (auto partner : partners) {
      partner_addr_terminated_runtime_indices_map[partner.val].push_back(index);
    }
    index += 1;
  }

  for (const auto &[partner_addr, _] : partner_addr_terminated_runtime_indices_map) {
    auto terminated_runtime_indices = partner_addr_terminated_runtime_indices_map[partner_addr];
    auto bell_pair_range = bell_pair_store.getBellPairsRange((QNIC_type)qnic_type, qnic_index, partner_addr);

    for (auto it = bell_pair_range.first; it != bell_pair_range.second; ++it) {
      auto sequence_number_qubit_record = it->second;
      auto sequence_number = sequence_number_qubit_record.first;
      auto qubit_record = sequence_number_qubit_record.second;
      auto ruleset_id = sequence_number_ruleset_id_map[sequence_number];
      if (qubit_record->isAllocated()) {
        qubit_record->setAllocated(false);
        auto terminated_runtimes = runtimes.getTerminatedRuntimes();
        for (auto rt = terminated_runtimes.begin(); rt != terminated_runtimes.end(); ++rt) {
          if (rt->ruleset.id == ruleset_id) {
            rt->freeQubitFromRuleSet(partner_addr, qubit_record);
            auto runtime_index = sequence_number_runtime_index_map[sequence_number];
            runtime_index_bell_pair_number_map[runtime_index] = runtime_index_bell_pair_number_map[runtime_index] - 1;
          }
        }
      }
    }
  }
}

void RuleEngine::sendConnectionTeardownNotifier(std::vector<unsigned long> ruleset_id_list) {
  ConnectionTeardownNotifier *pkt = new ConnectionTeardownNotifier("ConnectionTeardownNotifier");
  pkt->setSrcAddr(parentAddress);
  pkt->setDestAddr(parentAddress);
  for (auto ruleset_id : ruleset_id_list) {
    pkt->appendRuleSetId(ruleset_id);
  }
  send(pkt, "RouterPort$o");
}

void RuleEngine::executeAllRuleSets() {
  for (auto it = runtimes.begin(); it != runtimes.end(); ++it) {
    for (auto partner_address : it->partners) {
      std::vector<unsigned long> next_link_allocations;
      if (node_address_next_link_allocations_map.find(partner_address.val) != node_address_next_link_allocations_map.end()) {
        for (auto next_link_allocation : node_address_next_link_allocations_map[partner_address.val]) {
          node_address_active_link_allocations_map[partner_address.val].push_back(next_link_allocation);
        }
      }
      node_address_next_link_allocations_map[partner_address.val].clear();
    }
  }

  runtimes.exec();
  auto terminated_ruleset_id_list = runtimes.getTerminatedRuleSetIDs();
  if (terminated_ruleset_id_list.size() != 0) {
    sendConnectionTeardownNotifier(terminated_ruleset_id_list);
  }
}

void RuleEngine::freeConsumedResource(int qnic_index /*Not the address!!!*/, IStationaryQubit *qubit, QNIC_type qnic_type) {
  auto *qubit_record = qnic_store->getQubitRecord(qnic_type, qnic_index, qubit->par("stationary_qubit_address"));
  realtime_controller->ReInitialize_StationaryQubit(qubit_record, false);
  qubit_record->setBusy(false);
  if (qubit_record->isAllocated()) {
    qubit_record->setAllocated(false);
  }
  bell_pair_store.eraseQubit(qubit_record);
}

}  // namespace quisp::modules
