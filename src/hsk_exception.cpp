#include "hsk_exception.hpp"

namespace hsk {
    void Exception::Throw() { throw Exception(); }
    void Exception::Throw(std::string_view reason) { throw Exception(reason); }
}  // namespace hsk