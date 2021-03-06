#include "tests.hpp"
#include "lib_modules/modules.hpp"

#include "lib_media/decode/libav_decode.hpp"
#include "lib_media/demux/libav_demux.hpp"
#include "lib_media/out/print.hpp"
#include "lib_utils/tools.hpp"


using namespace Tests;
using namespace Modules;

namespace {

unittest("packet type erasure + multi-output: libav Demux -> {libav Decoder -> Out::Print}*") {
	auto demux = uptr(create<Demux::LibavDemux>("data/beepbop.mp4"));

	std::vector<std::unique_ptr<Decode::LibavDecode>> decoders;
	std::vector<std::unique_ptr<Out::Print>> printers;
	for (size_t i = 0; i < demux->getNumOutputs(); ++i) {
		auto metadata = getMetadataFromOutput<MetadataPktLibav>(demux->getOutput(i));
		auto decode = uptr(create<Decode::LibavDecode>(*metadata));

		auto p = uptr(create<Out::Print>(std::cout));

		ConnectOutputToInput(demux->getOutput(i), decode);
		ConnectOutputToInput(decode->getOutput(0), p);

		decoders.push_back(std::move(decode));
		printers.push_back(std::move(p));
	}

	demux->process(nullptr);
}

}
