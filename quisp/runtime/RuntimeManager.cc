#include "RuntimeManager.h"
#include <omnetpp/simtime.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#include "RuleSet.h"
#include "Runtime.h"
#include "omnetpp/cexception.h"
#include "runtime/RuleSet.h"
#include "runtime/types.h"

namespace quisp::runtime {

RuntimeManager::RuntimeManager(std::unique_ptr<Runtime::ICallBack> &&callback) : callback(std::move(callback)) {}

void RuntimeManager::acceptRuleSet(const RuleSet &ruleset) { runtimes.emplace_back(runtime::Runtime(ruleset, callback.get())); }

std::vector<Runtime>::iterator RuntimeManager::findById(unsigned long long ruleset_id) {
  for (auto it = runtimes.begin(); it != runtimes.end(); ++it) {
    if (it->ruleset.id == ruleset_id) {
      return it;
    }
  }
  return runtimes.end();
}

void RuntimeManager::exec() {
  terminated_ruleset_id_list.clear();
  for (auto it = runtimes.begin(); it != runtimes.end();) {
    auto ruleset_id = it->ruleset.id;
    if (!it->is_active && !it->is_terminated) {
      it->is_active = true;
    }
    it->exec();
    std::cout << it->return_code << std::endl;
    if (it->is_active && it->is_terminated) {
      terminated_ruleset_id_list.push_back(ruleset_id);
      it->is_active = false;
      terminated_runtimes.push_back(*it);
      it = runtimes.erase(it);
    } else {
      ++it;
    }
  }
}

std::vector<Runtime> RuntimeManager::getTerminatedRuntimes() { return terminated_runtimes; }

std::vector<unsigned long> RuntimeManager::getTerminatedRuleSetIDs() { return terminated_ruleset_id_list; }

void RuntimeManager::stopById(unsigned long long ruleset_id) {
  auto it = findById(ruleset_id);
  if (it->is_active) {
    it->is_active = false;
  }
  it->is_terminated = true;
}
std::vector<QNodeAddr> RuntimeManager::findPartnersById(unsigned long long ruleset_id) {
  for (auto &[k, v] : ruleset_id_partners_map) {
    if (k == ruleset_id) {
      return v;
    }
  }
  std::vector<QNodeAddr> partners;
  return partners;
}

std::vector<Runtime>::iterator RuntimeManager::begin() { return runtimes.begin(); }
std::vector<Runtime>::iterator RuntimeManager::end() { return runtimes.end(); }
std::vector<Runtime>::reference RuntimeManager::at(size_t index) { return runtimes.at(index); }
size_t RuntimeManager::size() const { return runtimes.size(); }

}  // namespace quisp::runtime
