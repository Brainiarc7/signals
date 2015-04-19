#include "tests.hpp"
#include "lib_modules/modules.hpp"

#include "lib_media/demux/libav_demux.hpp"
#include "lib_media/mux/gpac_mux_mp4.hpp"
#include "lib_utils/tools.hpp"


using namespace Tests;
using namespace Modules;

namespace {
size_t ConnectOutputToInput(IOutput* out, Module * const module, size_t outputIdx) {
	Modules::IInput *input = new Input<DataBase>(module);
	auto input = module->addInput(input);
	ASSERT((void*)input == (void*)module->getInput(outputIdx));
	return ConnectOutputToInput(out, module->getInput(outputIdx));
}

unittest("remux test: GPAC mp4 mux") {
	auto demux = uptr(new Demux::LibavDemux("data/BatmanHD_1000kbit_mpeg.mp4"));
	auto mux = uptr(new Mux::GPACMuxMP4("output_video_libav"));
	for (size_t i = 0; i < demux->getNumOutputs(); ++i) {
		ConnectOutputToInput(demux->getOutput(i), mux.get(), i);
		break;
	}

	demux->process(nullptr);
}

unittest("multiple inputs: send same packets to 2 GPAC mp4 mux inputs") {
	//TODO
}

}