#include <string>

namespace mce {
struct UUID {
  uint64_t most, least;
  UUID() {}
  const std::string asString() const;
  static mce::UUID fromString(std::string const &);
};
} // namespace mce