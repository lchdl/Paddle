// Copyright (c) 2021 CINN Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "paddle/cinn/poly/domain.h"

#include <glog/logging.h>

#include <algorithm>
#include <functional>
#include <numeric>
#include <sstream>
#include <unordered_set>

#include "paddle/cinn/common/context.h"
#include "paddle/cinn/ir/ir_visitor.h"
#include "paddle/cinn/utils/string.h"

namespace cinn {
namespace poly {

std::string Domain::__str__() const {
  CHECK(!id.empty()) << "domain's id is empty";
  std::vector<std::string> range_fields;
  std::transform(dims.begin(),
                 dims.end(),
                 std::back_inserter(range_fields),
                 [](const Dim& x) { return x.range_repr(); });
  std::string range_repr = utils::Join(range_fields, " and ");

  std::vector<std::string> dim_fields;
  std::transform(dims.begin(),
                 dims.end(),
                 std::back_inserter(dim_fields),
                 [](const Dim& x) { return x.id; });
  std::string dims_repr = utils::Join(dim_fields, ", ");

  // parameters
  std::vector<std::string> param_reprs;
  std::transform(params.begin(),
                 params.end(),
                 std::back_inserter(param_reprs),
                 [](const Dim& x) { return x.id; });
  std::string params_repr = utils::Join(param_reprs, ", ");

  return utils::StringFormat("[%s]->{ %s[%s]: %s }",
                             params_repr.c_str(),
                             id.c_str(),
                             dims_repr.c_str(),
                             range_repr.c_str());
}

isl::set Domain::to_isl() const {
  VLOG(3) << "isl::set " << __str__();
  isl::set x(common::Context::isl_ctx(), __str__());
  return x;
}

void Domain::ExtractParams() {
  std::unordered_set<std::string> var_names;
  auto collect_param_fn = [&](Expr& e) {
    if (!e.is_constant()) {
      auto vars = ir::ir_utils::CollectIRNodes(
          e, [](const Expr* e) { return e->is_var(); });
      for (auto& var : vars) var_names.insert(var.As<ir::_Var_>()->name);
    }
  };

  for (auto& dim : dims) {
    collect_param_fn(dim.lower_bound);
    collect_param_fn(dim.upper_bound);
  }

  for (auto& id : var_names) params.emplace_back(id);
}

}  // namespace poly
}  // namespace cinn
