#include "print.hpp"
#include "lib_utils/log.hpp"


namespace Modules {
namespace Out {

void Print::process(std::shared_ptr<const Data> data_) {
	auto data = safe_cast<const RawData>(data_);
	os << "Print: Received data of size: " << data->size() << std::endl;
}

Print::Print(std::ostream &os) : os(os) {
}

}
}