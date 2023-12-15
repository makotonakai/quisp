/** \file RuleEngine.h
 *
 *  \brief RuleEngine
 */
#pragma once

#include <map>
#include <unordered_map>
#include <vector>

#include <omnetpp.h>

#include "BellPairStore/BellPairStore.h"
#include "IRuleEngine.h"
#include "QNicStore/IQNicStore.h"
#include "QubitRecord/IQubitRecord.h"
#include "messages/barrier_messages_m.h"
#include "messages/classical_messages.h"
#include "messages/connection_teardown_messages_m.h"
#include "messages/link_allocation_update_messages_m.h"
#include "modules/Logger/LoggerBase.h"
#include "modules/QNIC.h"
#include "modules/QRSA/HardwareMonitor/IHardwareMonitor.h"
#include "modules/QRSA/RealTimeController/IRealTimeController.h"
#include "modules/QRSA/RoutingDaemon/IRoutingDaemon.h"
#include "omnetpp/cmessage.h"
#include "rules/RuleSet.h"
#include "runtime/Runtime.h"
#include "runtime/RuntimeManager.h"
#include "utils/ComponentProvider.h"

using namespace std;
using namespace omnetpp;
using namespace quisp::rules;

namespace quisp::modules::runtime_callback {
struct RuntimeCallback;
}

namespace quisp::modules {
using qnic_store::IQNicStore;
using qubit_record::IQubitRecord;

struct SwappingResultData {
  unsigned long ruleset_id;
  int shared_tag;
  int new_partner_addr;
  int operation_type;
  int qubit_index;
};

/** \class RuleEngine RuleEngine.h
 *  \note The Connection Manager responds to connection requests received from other nodes.
 *        Connection setup, so a regular operation but not high bandwidth, relatively low constraints.
 *        Connections from nearest neighbors only.
 *        Connection manager needs to know which qnic is connected to where, which QNode not BSA/EPPS.
 *
 *  \brief RuleEngine
 */

class RuleEngine : public IRuleEngine, public Logger::LoggerBase {
 public:
  friend runtime_callback::RuntimeCallback;
  RuleEngine();
  ~RuleEngine();
  int parentAddress;  // Parent QNode's address

  messages::EmitPhotonRequest *emt;
  int number_of_qnics_all;  // qnic,qnic_r,_qnic_rp
  int number_of_qnics;
  int number_of_qnics_r;
  int number_of_qnics_rp;
  std::map<unsigned long, vector<int>> ruleset_id_node_addresses_along_path_map;
  std::map<unsigned long, unsigned long> current_ruleset_id_next_ruleset_id_map;
  std::map<unsigned long, vector<int>> ruleset_id_qnic_addresses_map;
  std::vector<int> qnode_indices;

  IHardwareMonitor *hardware_monitor;
  IRoutingDaemon *routingdaemon;
  IRealTimeController *realtime_controller;
  BellPairStore bell_pair_store;

  void freeConsumedResource(int qnic_index, IStationaryQubit *qubit, QNIC_type qnic_type);
  void allocateBellPairs(int qnic_type, int qnic_index);
  void handleConnectionTeardownMessage(messages::InternalConnectionTeardownMessage *msg);
  void stopRuleSetExecution(messages::InternalConnectionTeardownMessage *msg);
  void addAllocatedQNICs(messages::InternalConnectionTeardownMessage *msg);
  void sendConnectionTeardownMessageForRuleSet(unsigned long ruleset_id);
  void sendLinkAllocationUpdateMessage(messages::LinkAllocationUpdateNotifier *msg);
  void storeInfoAboutIncomingLinkAllocationUpdateMessage(messages::LinkAllocationUpdateMessage *msg);
  void syncNextLinkAllocations();
  void sendBarrierMessage(int src_addr);
  void waitForBellPairGeneration(int src_addr);
  void keepWaitingForBellPairGeneration(messages::WaitMessage *msg);
  void sendLinkAllocationUpdateMessageForConnectionTeardown(messages::InternalConnectionTeardownMessage *msg);
  std::vector<unsigned long long> getActiveLinkAllcations();
  void executeAllRuleSets();
  unsigned long getRuleSetIdBySequenceNumber(int sequence_number);

 protected:
  void initialize() override;
  void handleMessage(cMessage *msg) override;
  void handleLinkGenerationResult(messages::CombinedBSAresults *bsa_result);
  void handlePurificationResult(messages::PurificationResult *purification_result);
  void handleSwappingResult(messages::SwappingResult *swapping_result);
  void sendEmitPhotonSignalToQnic(QNIC_type qnic_type, int qnic_index, int qubit_index, bool is_first, bool is_last);
  void stopOnGoingPhotonEmission(QNIC_type qnic_type, int qnic_index);
  void freeFailedEntanglementAttemptQubits(QNIC_type qnic_type, int qnic_index);
  simtime_t getEmitTimeFromBSMNotification(messages::BSMTimingNotification *notification);
  void schedulePhotonEmission(QNIC_type qnic_type, int qnic_index, messages::BSMTimingNotification *notification);

  utils::ComponentProvider provider;
  std::unique_ptr<IQNicStore> qnic_store = nullptr;

  runtime::RuntimeManager runtimes;
  std::unordered_map<std::pair<QNIC_type, int>, messages::EmitPhotonRequest *> emit_photon_timer_map;
  std::unordered_map<std::pair<QNIC_type, int>, std::vector<int>> emitted_photon_order_map;
  std::unordered_map<int, std::vector<unsigned long>> node_address_active_link_allocation_map;
  std::unordered_map<int, std::vector<unsigned long>> node_address_next_link_allocation_map;
  std::unordered_map<int, bool> node_address_lau_sent_map;
  std::unordered_map<int, bool> node_address_lau_responded_map;
  std::vector<unsigned long> incoming_active_link_allocations;
  std::vector<unsigned long> incoming_next_link_allocations;
  int incoming_random_number;
  std::vector<unsigned long> active_link_allocations;
  std::vector<unsigned long> next_link_allocations;
  int random_number;
  bool lau_sent = false;
  bool lau_received = false;
};

Define_Module(RuleEngine);
}  // namespace quisp::modules
