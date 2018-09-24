#include <minecraft/net/UUID.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/endian/conversion.hpp>

static boost::uuids::string_generator gen;

mce::UUID mce::UUID::fromStringFix(std::string const &str) {
  auto temp = gen(str);
  auto t2   = (UUID *)(&temp);
  boost::endian::endian_reverse_inplace(t2->most);
  boost::endian::endian_reverse_inplace(t2->least);
  return *t2;
}