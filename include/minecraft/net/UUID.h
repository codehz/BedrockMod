#include <string>

namespace mce {
struct UUID {
  uint64_t most, least;
  UUID() {}
  const std::string asString() const;
  void fromString(std::string const &);
};
} // namespace mce