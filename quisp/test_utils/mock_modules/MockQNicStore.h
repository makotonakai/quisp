#pragma once

#include <gmock/gmock.h>
#include <modules/QNIC.h>
#include <modules/QRSA/RuleEngine/QNicStore/IQNicStore.h>
#include <test_utils/TestUtilFunctions.h>

namespace quisp_test::mock_modules::qnic_store {

using quisp::modules::QNIC_type;
using quisp::modules::qnic_store::IQNicStore;

class MockQNicStore : public IQNicStore {
 public:
  MOCK_METHOD(int, countNumFreeQubits, (QNIC_type type, int qnic_index), (override));
  MOCK_METHOD(int, takeFreeQubitIndex, (QNIC_type type, int qnic_index), (override));
  MOCK_METHOD(void, setQubitBusy, (QNIC_type type, int qnic_index, int qubit_index, bool is_busy), (override));
  MOCK_METHOD(quisp::modules::qrsa::IQubitRecord*, getQubitRecord, (QNIC_type type, int qnic_index, int qubit_index), (override));
};
}  // namespace quisp_test::mock_modules::qnic_store
