#include "../../tests/tests.hpp"
#include "modules.hpp"

#include "libavcodec/avcodec.h" //FIXME: there should be none of the modules include at the application level

#include "decode/libav_decode.hpp"
#include "demux/libav_demux.hpp"
#include "encode/libav_encode.hpp"
#include "mux/gpac_mux_mp4.hpp"
#include "stream/mpeg_dash.hpp"
#include "transform/audio_convert.hpp"
#include "transform/video_convert.hpp"
#include "out/null.hpp"

#include "../../utils/tools.hpp"

#include <atomic>
#include <sstream>

using namespace Tests;
using namespace Modules;


namespace {

Encode::LibavEncode* createEncoder(PropsDecoder *decoderProps) {
	auto const codecType = decoderProps ? decoderProps->getAVCodecContext()->codec_type : AVMEDIA_TYPE_UNKNOWN;
	if (codecType == AVMEDIA_TYPE_VIDEO) {
		Log::msg(Log::Info, "[Encoder] Found video stream");
		return new Encode::LibavEncode(Encode::LibavEncode::Video);
	} else if (codecType == AVMEDIA_TYPE_AUDIO) {
		Log::msg(Log::Info, "[Encoder] Found audio stream");
		return new Encode::LibavEncode(Encode::LibavEncode::Audio);
	} else {
		Log::msg(Log::Info, "[Encoder] Found unknown stream");
		return nullptr;
	}
}

Module* createConverter(PropsDecoder *decoderProps) {
	auto const codecType = decoderProps ? decoderProps->getAVCodecContext()->codec_type : AVMEDIA_TYPE_UNKNOWN;
	if (codecType == AVMEDIA_TYPE_VIDEO) {
		Log::msg(Log::Info, "[Converter] Found video stream");
		auto srcCtx = decoderProps->getAVCodecContext();
		auto srcRes = Resolution(srcCtx->width, srcCtx->height);
		auto dstRes = Resolution(320, 180);
		return new Transform::VideoConvert(srcRes, srcCtx->pix_fmt, dstRes, srcCtx->pix_fmt);
	} else if (codecType == AVMEDIA_TYPE_AUDIO) {
		Log::msg(Log::Info, "[Converter] Found audio stream");
		auto baseFormat = PcmFormat(44100, 2, AudioLayout::Stereo, AudioSampleFormat::F32, AudioStruct::Planar);
		auto otherFormat = PcmFormat(44100, 2, AudioLayout::Stereo, AudioSampleFormat::S16, AudioStruct::Interleaved);
		return new Transform::AudioConvert(baseFormat, otherFormat);
	} else {
		Log::msg(Log::Info, "[Converter] Found unknown stream");
		return nullptr;
	}
}
}


int safeMain(int argc, char const* argv[]) {
	if (argc != 2)
		throw std::runtime_error("usage: dashcastx <URL>");

	auto const inputURL = argv[1];

	Tools::Profiler profilerGlobal("Dashcast X");

	Pipeline pipeline;

	auto demux = pipeline.addModule(Demux::LibavDemux::create(inputURL), true);
	auto dasher = pipeline.addModule(new Modules::Stream::MPEG_DASH(Modules::Stream::MPEG_DASH::Static));

	for (size_t i = 0; i < demux->getNumPin(); ++i) {
		auto props = demux->getPin(i)->getProps();
		auto decoderProps = safe_cast<PropsDecoder>(props);

		auto decoder = pipeline.addModule(new Decode::LibavDecode(*decoderProps));
		pipeline.connect(demux->getPin(i), decoder);

		auto converter = pipeline.addModule(createConverter(decoderProps));
		if (!converter) {
			continue;
		}

		pipeline.connect(decoder->getPin(0), converter);

		auto rawEncoder = createEncoder(decoderProps);
		auto encoder = pipeline.addModule(rawEncoder);
		if (!encoder) {
			continue;
		}

		pipeline.connect(converter->getPin(0), encoder);

		std::stringstream filename;
		filename << i;
		auto muxer = pipeline.addModule(new Mux::GPACMuxMP4(filename.str(), true));

		pipeline.connect(encoder->getPin(0), muxer);
		pipeline.connect(muxer->getPin(0), dasher);

		rawEncoder->sendOutputPinsInfo();
	}

	{
		Tools::Profiler profilerProcessing("Dashcast X - processing time");
		pipeline.start();
		pipeline.waitForCompletion();
	}

	return 0;
}

int main(int argc, char const* argv[]) {
	try {
		return safeMain(argc, argv);
	} catch(std::exception const& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}