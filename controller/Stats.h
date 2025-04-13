#pragma once

#include <array>
#include <cstdint>

namespace Stats {

enum class Type : std::uint8_t {
  // Status information
  Rtt = 0,
  ResendTimeout,
  ResentPackets,
  Uptime,
  LoadAvg,
  WlanStrength,
  Distance,
  Temperature,
  Current,
  Voltage,
  Turn,
  Speed,

  // Network stats
  PayloadRx,
  PayloadTx,
  TotalRx,
  TotalTx,

  // This must be the last item
  Count
};

class Container {
public:
  Container() : values{} {}

  // Access by enum
  int& operator[](Type type) {
    return values[static_cast<std::size_t>(type)];
  }

  const int& operator[](Type type) const {
    return values[static_cast<std::size_t>(type)];
  }

  // Get raw data pointer for efficient passing
  int* data() { return values.data(); }
  const int* data() const { return values.data(); }

  // Get size
  static constexpr std::size_t size() {
    return static_cast<std::size_t>(Type::Count);
  }

private:
  std::array<int, static_cast<std::size_t>(Type::Count)> values;
};

} // namespace Stats