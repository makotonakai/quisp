#pragma once

#include <modules/Logger/ILogger.h>
#include <modules/QNIC.h>
#include <modules/QNIC/StationaryQubit/IStationaryQubit.h>
#include <modules/QRSA/QRSA.h>

namespace quisp::modules {

using QNodeAddr = int;
using QNicIndex = int;
// entangled partner qnode address -> qubit
using SequenceNumberQubit = std::pair<int, qrsa::IQubitRecord*>;
using PartnerAddrSequenceNumberQubitMap = std::multimap<QNodeAddr, SequenceNumberQubit>;
using ResourceKey = std::pair<QNIC_type, QNicIndex>;
using PartnerAddrSequenceNumberQubitMapRange = std::pair<PartnerAddrSequenceNumberQubitMap::iterator, PartnerAddrSequenceNumberQubitMap::iterator>;

/**
 * this class contains the bell pair information for RuleEngine.
 * this tracks the entangled qubit and its partner addr.
 * RuleEngine recognizes a bell pair generated, store the information to this class.
 * if RuleSet needs bell pair resource, RuleEngine takes a bell pair from this class.
 */
class BellPairStore {
 public:
  BellPairStore(Logger::ILogger* logger = nullptr);
  ~BellPairStore();
  void eraseQubit(qrsa::IQubitRecord* const qubit);
  void insertEntangledQubit(int sequence_number, QNodeAddr partner_addr, qrsa::IQubitRecord* qubit);
  qrsa::IQubitRecord* findQubit(int sequence_number, QNIC_type qnic_type, QNicIndex qnic_index, QNodeAddr addr);
  PartnerAddrSequenceNumberQubitMapRange getBellPairsRange(QNIC_type qnic_type, QNicIndex qnic_index, QNodeAddr partner_addr);
  int getFirstAvailableSequenceNumber();
  bool bellPairExist(QNIC_type qnic_type, int qnic_index, int partner_addr);
  qrsa::IQubitRecord* findFirstFreeQubitRecordBySequenceNumberAndPartnerAddress(int sequence_number, int addr);
  qrsa::IQubitRecord* allocateFirstAvailableQubitRecord(int sequence_number, int addr);
  int getQnicIndexByNumberOfQnicsAndPartnerAddress(int number_of_qnics, int partner_addr);
  std::string toString() const;
  Logger::ILogger* logger;

 protected:
  std::map<ResourceKey, PartnerAddrSequenceNumberQubitMap> _resources;
  std::map<int, bool> sequence_number_is_allocated_map;
};
std::ostream& operator<<(std::ostream& os, const quisp::modules::BellPairStore& store);
}  // namespace quisp::modules
