#pragma once
#include <string>
#include "hart.h"

bool load_elf(Hart& hart, const std::string& filename);