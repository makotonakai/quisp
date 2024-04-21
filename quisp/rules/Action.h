#pragma once
#include <nlohmann/json.hpp>

#include "modules/QNIC.h"
#include "modules/QUBIT.h"

using nlohmann::json;
using quisp::modules::QNIC_type;
namespace quisp::rules {

struct QnicInterface {
  int partner_addr;
};

enum PurType : int {
  INVALID,  ///< Invalid purification type
  Single_Selection_X_Purification,  ///< Single purification for X error
  Single_Selection_Z_Purification,  ///< Single purification for Z error
  Single_Selection_Y_Purification,
  Single_Selection_XZ_Purification,  ///< Double purification both for X and Z errors
  Single_Selection_ZX_Purification,  ///< Double inverse purification both for X and Z errors
  Double_Selection_X_Purification,  ///< Double selection XZ and single action (DoubleSelectionAction) for X error
  Double_Selection_Z_Purification,  ///< Inverse Double selection XZ and single action(DoubleSelectionAction) for X error
  Double_Selection_XZ_Purification,  ///< Double Selection and Dual Action for both X and Z errors
  Double_Selection_ZX_Purification,  ///< Inverse Double Selection and Dual Action for both X and Z errors
  Double_Selection_X_Purification_Single_Selection_Z_Purification,  ///< Different type of Double Selection and Dual Action for both X and Z errors
  Double_Selection_Z_Purification_Single_Selection_X_Purification,  ///< Different type of Inverse Double Selection and Dual Action for both X and Z errors
};

NLOHMANN_JSON_SERIALIZE_ENUM(PurType, {
                                          {INVALID, "INVALID"},
                                          {Single_Selection_X_Purification, "Single_Selection_X_Purification"},
                                          {Single_Selection_Z_Purification, "Single_Selection_Z_Purification"},
                                          {Single_Selection_Y_Purification, "Single_Selection_Y_Purification"},
                                          {Single_Selection_XZ_Purification, "Single_Selection_XZ_Purification"},
                                          {Single_Selection_ZX_Purification, "Single_Selection_ZX_Purification"},
                                          {Double_Selection_X_Purification, "Double_Selection_X_Purification"},
                                          {Double_Selection_Z_Purification, "Double_Selection_Z_Purification"},
                                          {Double_Selection_XZ_Purification, "Double_Selection_XZ_Purification"},
                                          {Double_Selection_ZX_Purification, "Double_Selection_ZX_Purification"},
                                          {Double_Selection_X_Purification_Single_Selection_Z_Purification, "Double_Selection_X_Purification_Single_Selection_Z_Purification"},
                                          {Double_Selection_Z_Purification_Single_Selection_X_Purification, "Double_Selection_Z_Purification_Single_Selection_X_Purification"},
                                      })

inline void to_json(json& j, const QnicInterface& qi) { j = json{{"partner_address", qi.partner_addr}}; }

inline void from_json(const json& j, QnicInterface& qi) { j.at("partner_address").get_to(qi.partner_addr); }
class Action {
 public:
  Action() {}  // for deserialization
  Action(int partner_addr);
  Action(std::vector<int> partner_addr);
  virtual ~Action() {}
  std::vector<QnicInterface> qnic_interfaces;
  int partner_address;
  virtual json serialize_json() = 0;
  virtual void deserialize_json(json serialized) = 0;
};

class Purification : public Action {
 public:
  Purification(json serialized) { deserialize_json(serialized); }  // for deserialization
  Purification(PurType purification_type, int partner_addr, int shared_rule_tag);
  PurType purification_type;
  int shared_rule_tag;
  json serialize_json() override;
  void deserialize_json(json serialized) override;
};

class EntanglementSwapping : public Action {
 public:
  EntanglementSwapping(json serialized) { deserialize_json(serialized); }  // for deserialization
  EntanglementSwapping(std::vector<int> partner_addr, int shared_rule_tag);
  std::vector<QnicInterface> remote_qnic_interfaces;
  int shared_rule_tag;
  json serialize_json() override;
  void deserialize_json(json serialized) override;
};

class PurificationCorrelation : public Action {
 public:
  PurificationCorrelation(json serialized) { deserialize_json(serialized); }  // for deserialization
  PurificationCorrelation(int partner_addr, int shared_rule_tag);
  int shared_rule_tag;
  json serialize_json() override;
  void deserialize_json(json serialized) override;
};

class SwappingCorrection : public Action {
 public:
  SwappingCorrection(json serialized) { deserialize_json(serialized); }  // for deserialization
  SwappingCorrection(int swapper_addr, int shared_rule_tag);
  int shared_rule_tag;
  json serialize_json() override;
  void deserialize_json(json serialized) override;
};

class Tomography : public Action {
 public:
  Tomography(json serialized) { deserialize_json(serialized); }  // for deserialization
  Tomography(int num_measurement, int owner_addr, int partner_addr);
  simtime_t start_time = -1;
  int num_measurement;
  int owner_address;
  json serialize_json() override;
  void deserialize_json(json serialized) override;
};

}  // namespace quisp::rules
