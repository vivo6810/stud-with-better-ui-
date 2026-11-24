#pragma once

#include "mac_client.h"

#include <optional>

namespace stud::ui {

// Player launcher home; empty when closed without launching.
std::optional<client::Product> choose_product();

}  // namespace stud::ui
