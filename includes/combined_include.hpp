#pragma once

// standard io
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdio>

// containers
#include <string>
#include <vector>
#include <array>
#include <deque>
#include <queue>
#include <stack>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>

// utilities
#include <utility>
#include <optional>
#include <variant>
#include <tuple>
#include <functional>
#include <memory>
#include <regex>

// algorithms
#include <algorithm>
#include <numeric>

// char/string handling
#include <cctype>
#include <cstring>
#include <string_view>

// numbers/types
#include <cstdint>
#include <cstddef>
#include <limits>
#include <cmath>

// time/random
#include <ctime>
#include <chrono>
#include <random>

// errors/debug
#include <exception>
#include <stdexcept>
#include <cassert>

// filesystem/system
#include <filesystem>
#include <cstdlib>

// threading (optional but useful later)
#include <thread>
#include <mutex>
#include <atomic>
#include <span>

#include "json.hpp"

using json = nlohmann::json;