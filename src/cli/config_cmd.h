#pragma once

#include <string>

namespace bagu::cli {

int run_config_get(const std::string& key);
int run_config_set(const std::string& key, const std::string& value);
int run_config_list();
int run_config_list_profiles();

}  // namespace bagu::cli
