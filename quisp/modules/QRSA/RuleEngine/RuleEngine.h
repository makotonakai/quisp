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
#include "messages/BSA_ipc_messages_m.h"
#include "messages/classical_messages.h"
#include "messages/link_generation_messages_m.h"
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
  void allocateBellPairs(int qnic_type, int qnic_index, int first_sequence_number);
  void releaseBellPairs(int qnic_type, int qnic_index, int first_sequence_number);
  void handleInternalConnectionTeardownMessage(messages::InternalConnectionTeardownMessage *msg);
  void stopRuleSetExecution(messages::InternalConnectionTeardownMessage *msg);
  void removeRuleSetIdFromActiveLinkAllocationPolicy(unsigned long ruleset_id);
  void sendLinkAllocationUpdateMessages();
  void storeInfoAboutIncomingLinkAllocationUpdateMessage(messages::LinkAllocationUpdateMessage *msg);
  void negotiateNextLinkAllocationPolicy(int src_addr);
  void sendBarrierMessage(int src_addr);
  void waitForBellPairGeneration(int src_addr);
  void keepWaitingForBellPairGeneration(messages::WaitMessage *msg);
  std::vector<unsigned long long> getActiveLinkAllcations();
  void executeAllRuleSets();
  unsigned long getRuleSetIdBySequenceNumber(int sequence_number);
  bool bellPairExist();
  void storeInfoAboutBarrierMessage(messages::BarrierMessage *msg);
  void negotiateNextSequenceNumber(int src_addr);
  void sendConnectionTeardownNotifier(std::vector<unsigned long> ruleset_id_list);

 protected:
  void initialize() override;
  void handleMessage(cMessage *msg) override;
  void handleMSMResult(messages::MSMResult *msm_result);
  void handleLinkGenerationResult(messages::CombinedBSAresults *bsa_result);
  void handlePurificationResult(messages::PurificationResult *purification_result);
  void handleSwappingResult(messages::SwappingResult *swapping_result);
  void handleSingleClickResult(messages::SingleClickResult *click_result);
  messages::CombinedBSAresults *generateCombinedBSAresults(int qnic_index);
  void executeAllRuleSets();
  void sendEmitPhotonSignalToQnic(QNIC_type qnic_type, int qnic_index, int qubit_index, bool is_first, bool is_last);
  void stopOnGoingPhotonEmission(QNIC_type qnic_type, int qnic_index);
  void freeFailedEntanglementAttemptQubits(QNIC_type qnic_type, int qnic_index);
  simtime_t getEmitTimeFromBSMNotification(messages::BSMTimingNotification *notification);
  void schedulePhotonEmission(QNIC_type qnic_type, int qnic_index, messages::BSMTimingNotification *notification);
  void scheduleMSMPhotonEmission(QNIC_type qnic_type, int qnic_index, messages::EPPSTimingNotification *notification);
  void handleStopEmitting(messages::StopEmitting *stop_emit);

  utils::ComponentProvider provider;
  std::unique_ptr<IQNicStore> qnic_store = nullptr;

  runtime::RuntimeManager runtimes;
  std::unordered_map<std::pair<QNIC_type, int>, messages::EmitPhotonRequest *> emit_photon_timer_map;
  std::unordered_map<std::pair<QNIC_type, int>, std::vector<int>> emitted_photon_order_map;
  std::map<int, std::vector<unsigned long>> node_address_incoming_active_link_allocations_map;
  std::map<int, std::vector<unsigned long>> node_address_active_link_allocations_map;
  std::map<int, std::vector<unsigned long>> node_address_incoming_next_link_allocations_map;
  std::map<int, std::list<unsigned long>> node_address_next_link_allocations_map;
  std::map<int, int> node_address_incoming_random_number_map;
  std::map<int, int> node_address_random_number_map;
  std::map<int, bool> node_address_lau_sent_map;
  std::map<int, bool> node_address_lau_received_map;
  std::map<int, std::vector<int>> node_address_neighbor_addresses_map;
  std::map<int, bool> node_address_barrier_sent_map;
  std::map<int, bool> node_address_barrier_received_map;
  std::map<int, int> node_address_incoming_sequence_number_map;
  std::map<int, int> node_address_sequence_number_map;
  std::map<int, int> runtime_index_bell_pair_number_map;
  std::map<int, int> terminated_runtime_index_bell_pair_number_map;
  std::map<int, unsigned long> sequence_number_ruleset_id_map;
  std::map<int, unsigned long> sequence_number_runtime_index_map;
  std::set<int> partner_addresses;

  struct QubitInfo {
    int qubit_index;
    PauliOperator correction_operation = PauliOperator::I;
  };

  struct MSMInfo {
    int partner_address;
    int partner_qnic_index;
    int epps_address;
    unsigned long long photon_index_counter;
    int iteration_index;
    simtime_t total_travel_time;
    // map of iteration index and qubit index
    std::unordered_map<int, int> qubit_info_map;
    // map of photon index and qubit info
    std::unordered_map<int, QubitInfo> qubit_postprocess_info;
  };

  // [Key: qnic_index, Value: qubit_index]
  std::unordered_map<int, MSMInfo> msm_info_map;
};

Define_Module(RuleEngine);
}  // namespace quisp::modules
