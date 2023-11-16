#include "BellPairStore.h"
#include <sstream>
#include <utility>
#include "modules/QNIC.h"
#include "modules/QRSA/QRSA.h"

using quisp::modules::qubit_record::IQubitRecord;

namespace quisp::modules {
BellPairStore::BellPairStore(Logger::ILogger *logger) : logger(logger) {}
BellPairStore::~BellPairStore() {}

void BellPairStore::insertEntangledQubit(int sequence_number, QNodeAddr partner_addr, qrsa::IQubitRecord *const qubit) {
  auto qnic_type = qubit->getQNicType();
  auto qnic_index = qubit->getQNicIndex();
  ResourceKey key{qnic_type, qnic_index};
  logger->logBellPairInfo("Generated", partner_addr, qubit->getQNicType(), qubit->getQNicIndex(), qubit->getQubitIndex());
  auto sequence_number_qubit = std::pair<int, qrsa::IQubitRecord *>{std::make_pair(sequence_number, qubit)};
  if (_resources.find(key) == _resources.cend()) {
    _resources.emplace(key, std::multimap<int, SequenceNumberQubit>{std::make_pair(partner_addr, sequence_number_qubit)});
  } else {
    _resources[key].emplace(partner_addr, sequence_number_qubit);
  }
}

void BellPairStore::eraseQubit(qrsa::IQubitRecord *const qubit) {
  auto qnic_type = (QNIC_type)qubit->getQNicType();
  auto qnic_index = qubit->getQNicIndex();
  if (_resources.find(std::make_pair(qnic_type, qnic_index)) == _resources.cend()) {
    return;
  }
  // take the ref of entangled partner qubits map
  auto &resource = _resources[std::make_pair(qnic_type, qnic_index)];
  auto it = resource.begin();
  while (it != resource.cend()) {
    if (it->second.second == qubit) {
      logger->logBellPairInfo("Erased", it->second.first, qubit->getQNicType(), qubit->getQNicIndex(), qubit->getQubitIndex());
      it = resource.erase(it);
    } else
      it++;
  }
}

qrsa::IQubitRecord *BellPairStore::findQubit(int sequence_number, QNIC_type qnic_type, QNicIndex qnic_index, QNodeAddr addr) {
  auto key = std::make_pair(qnic_type, qnic_index);
  if (_resources.find(key) == _resources.cend()) {
    return nullptr;
  }
  auto it = _resources[key].find(addr);
  if (it == _resources[key].cend()) {
    return nullptr;
  }
  return it->second.second;
}

PartnerAddrSequenceNumberQubitMapRange BellPairStore::getBellPairsRange(QNIC_type qnic_type, int qnic_index, int partner_addr) {
  auto key = std::make_pair(qnic_type, qnic_index);
  if (_resources.find(key) == _resources.cend()) {
    _resources.emplace(key, std::multimap<int, SequenceNumberQubit>{});
  }
  return _resources[key].equal_range(partner_addr);
}

PartnerAddrSequenceNumberQubitMap::iterator BellPairStore::getFirstAvailableSequenceNumberQubit(QNodeAddr addr) {
  for (auto &[key, partner_sequence_number_qubit_map] : _resources) {
    auto qnic_index = key.second;
    auto range = getBellPairsRange(QNIC_E, qnic_index, addr);
    int count = 0;
    for (auto it = range.first; it != range.second; it++) {
      auto qubit_record = it->second.second;
      if (!qubit_record->isAllocated()) {
        return it;
      }
    }
  }
}

bool BellPairStore::bellPairExist(QNodeAddr addr) {
  for (auto &[key, partner_sequence_number_qubit_map] : _resources) {
    auto qnic_index = key.second;
    auto range = getBellPairsRange(QNIC_E, qnic_index, addr);
    int count = 0;
    for (auto it = range.first; it != range.second; it++) {
      count++;
    }
    if (count != 0) {
      return true;
    }
  }
  return false;
}

qrsa::IQubitRecord *BellPairStore::findFirstFreeQubitRecordBySequenceNumberAndPartnerAddress(int sequence_number, int addr) {
  for (auto &[key, partner_sequence_number_qubit_map] : _resources) {
    auto qnic_index = key.second;
    auto range = getBellPairsRange(QNIC_E, qnic_index, addr);
    int count = 0;
    for (auto it = range.first; it != range.second; it++) {
      if (it->second.first == sequence_number) {
        auto qubit_record = it->second.second;
        if (!qubit_record->isAllocated()) {
          return qubit_record;
        }
      }
    }
  }
}

void BellPairStore::allocateQubitRecord(int sequence_number, int addr, qrsa::IQubitRecord *qubit_record) {
  for (auto &[key, partner_sequence_number_qubit_map] : _resources) {
    auto qnic_index = key.second;
    auto range = getBellPairsRange(QNIC_E, qnic_index, addr);
    int count = 0;
    for (auto it = range.first; it != range.second; it++) {
      auto _sequence_number = it->second.first;
      auto _qubit_record = it->second.second;
      if (_sequence_number == sequence_number && _qubit_record == qubit_record && !_qubit_record->isAllocated()) {
        it->second.second->setAllocated(true);
      }
    }
  }
}

std::string BellPairStore::toString() const {
  std::stringstream ss;
  for (auto &[key, partner_sequence_number_qubit_map] : _resources) {
    for (auto &[partner, sequence_number_qubit_map] : partner_sequence_number_qubit_map) {
      ss << "(type:" << key.first << ", qnic:" << key.second << ", qubit:" << sequence_number_qubit_map.second->getQubitIndex() << ")=>(partner:" << partner << "), ";
    }
  }
  return ss.str();
}

std::ostream &operator<<(std::ostream &os, const quisp::modules::BellPairStore &store) {
  os << store.toString();
  return os;
}
}  // namespace quisp::modules
