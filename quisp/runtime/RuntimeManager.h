#pragma once
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

#include "Runtime.h"
#include "runtime/RuleSet.h"
using quisp::runtime::QNodeAddr;

namespace quisp::runtime {
class RuntimeManager {
 public:
  RuntimeManager(std::unique_ptr<Runtime::ICallBack>&& callback);
  void acceptRuleSet(const RuleSet&);
  std::vector<Runtime>::iterator findById(unsigned long long ruleset_id);
  std::vector<Runtime>::iterator findTerminatedRuntimeById(unsigned long long ruleset_id);
  std::vector<QNodeAddr> findPartnersById(unsigned long long ruleset_id);
  std::vector<unsigned long> getTerminatedRuleSetIDs();
  std::list<Runtime> getTerminatedRuntimes();
  void exec();
  void stopById(unsigned long long ruleset_id);
  std::vector<Runtime>::iterator begin();
  std::vector<Runtime>::iterator end();
  std::vector<Runtime>::reference at(size_t);
  size_t size() const;
  std::vector<unsigned long> terminated_ruleset_id_list;

 protected:
  std::vector<Runtime> runtimes;
  // terminated_runtimes is set to be a list instead of a vector in order to avoid iterator destruction
  std::list<Runtime> terminated_runtimes;
  std::unique_ptr<Runtime::ICallBack> callback;
  std::map<unsigned long, std::vector<QNodeAddr> > ruleset_id_partners_map;
};
}  // namespace quisp::runtime
