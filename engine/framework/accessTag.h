#pragma once

namespace fe::engine {

// ============================================================================
// AccessTag: Defines the access mode for resources
// ============================================================================

enum class AccessTag : uint8_t {
  None = 0,
  Read = 1 << 0,
  Write = 1 << 1,
  Exclusive = 1 << 2,
};

// Bitwise operators for AccessTag
inline constexpr AccessTag operator|(AccessTag a, AccessTag b) {
  return static_cast<AccessTag>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline constexpr AccessTag operator&(AccessTag a, AccessTag b) {
  return static_cast<AccessTag>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline constexpr bool has_tag(AccessTag value, AccessTag flag) {
  return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

inline constexpr bool is_write(AccessTag tag) {
  return has_tag(tag, AccessTag::Write) || has_tag(tag, AccessTag::Exclusive);
}

inline constexpr bool is_exclusive(AccessTag tag) {
  return has_tag(tag, AccessTag::Exclusive);
}

// ============================================================================
// Access conflict detection rules:
// - Exclusive conflicts with everything
// - Write conflicts with Write and Read on same resource
// - Read conflicts with Write on same resource
// - Read and Read do NOT conflict
// ============================================================================

inline constexpr bool has_access_conflict(AccessTag a, AccessTag b) {
  // Exclusive always conflicts
  if (is_exclusive(a) || is_exclusive(b)) {
    return true;
  }
  // Write vs anything (Read or Write) conflicts
  if (is_write(a) || is_write(b)) {
    return true;
  }
  // Read vs Read: no conflict
  return false;
}

}  // namespace fe::engine
