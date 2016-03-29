#include "mpeg_dash.hpp"
#include "lib_modules/core/clock.hpp"
#include "lib_utils/log.hpp"
#include "lib_utils/tools.hpp"
#include "../out/file.hpp"
#include "../common/libav.hpp"
#include <fstream>


#define MIN_BUFFER_TIME_IN_MS 3000

namespace Modules {
namespace Stream {

MPEG_DASH::MPEG_DASH(const std::string &mpdPath, Type type, uint64_t segDurationInMs)
	: mpdPath(mpdPath), type(type), segDurationInMs(segDurationInMs), totalDurationInMs(0),
	  mpd(new gpacpp::MPD(type == Live ? GF_MPD_TYPE_DYNAMIC : GF_MPD_TYPE_STATIC, MIN_BUFFER_TIME_IN_MS)) {
	addInput(new Input<DataAVPacket>(this));
}

void MPEG_DASH::endOfStream() {
	if (workingThread.joinable()) {
		for (size_t i = 0; i < inputs.size(); ++i)
			inputs[i]->push(nullptr);
		workingThread.join();
	}
}

MPEG_DASH::~MPEG_DASH() {
	endOfStream();
}

//needed because of the use of system time for live - otherwise awake on data as for any multi-input module
//TODO: add clock to the scheduler
void MPEG_DASH::DASHThread() {
	mpd->mpd->availabilityStartTime = gf_net_get_utc();

	Data data;
	for (;;) {
		Log::msg(Log::Warning, "[MPEG_DASH] Processing new segment (total processed: %ss).", (double)totalDurationInMs / 1000);

		meta.resize(getNumInputs() - 1);
		for (size_t i = 0; i < getNumInputs() - 1; ++i) {
			data = inputs[i]->pop();
			if (!data) {
				break;
			} else {
				meta[i] = safe_cast<const MetadataFile>(data->getMetadata());
				if (!meta[i])
					throw std::runtime_error(format("[MPEG_DASH] Unknown data received on input %s", i).c_str());
			}
		}
		if (!data)
			break;

		generateMPD(Live);

		if (type == Live) {
			auto dur = std::chrono::milliseconds(mpd->mpd->availabilityStartTime + totalDurationInMs - gf_net_get_utc());
			Log::msg(Log::Info, "[MPEG_DASH] Going to sleep for %s ms.", std::chrono::duration_cast<std::chrono::milliseconds>(dur).count());
			std::this_thread::sleep_for(dur);
		}
	}

	if (type == Static) {
		mpd->mpd->media_presentation_duration = (u32)totalDurationInMs; //FIXME: is u64 in latest GPAC
		generateMPD(Static);
	}
}

void MPEG_DASH::process() {
	if (!workingThread.joinable()) {
		numDataQueueNotify = (int)getNumInputs() - 1; //FIXME: connection/disconnection cannot occur dynamically. Lock inputs?
		workingThread = std::thread(&MPEG_DASH::DASHThread, this);
	}
}

void  MPEG_DASH::ensureMPD() {
	if (!gf_list_count(mpd->mpd->periods)) {
		auto period = mpd->addPeriod();
		for (size_t i = 0; i < getNumInputs() - 1; ++i) {
			auto as = mpd->addAdaptationSet(period);
			GF_SAFEALLOC(as->segment_template, GF_MPD_SegmentTemplate);
			as->segment_template->duration = segDurationInMs;
			as->segment_template->timescale = 1000;
			as->segment_template->media = gf_strdup(format("%s.mp4_$Number$", i).c_str());
			as->segment_template->initialization = gf_strdup(format("$RepresentationID$.mp4", i).c_str());
			as->segment_template->start_number = 0;

			//FIXME: arbitrary: should be set by the app, or computed
			as->segment_alignment = GF_TRUE;
			as->bitstream_switching = GF_TRUE;

			auto rep = mpd->addRepresentation(as, format("%s", i).c_str(), (u32)i * 100000/*FIXME: get bitrate*/);
			rep->mime_type = gf_strdup(meta[i]->getMimeType().c_str());
			rep->codecs = gf_strdup(meta[i]->getCodecName().c_str());
			switch (meta[i]->getStreamType()) {
			case AUDIO_PKT: rep->samplerate = meta[i]->sampleRate; break;
			case VIDEO_PKT: rep->width = meta[i]->resolution[0]; rep->height = meta[i]->resolution[1]; break;
			default: assert(0);
			}
		}
	}
}

void MPEG_DASH::generateMPD(Type ifType) {
	ensureMPD();
	if (ifType == type && !mpd->write(mpdPath))
		Log::msg(Log::Warning, "[MPEG_DASH] Can't write MPD at %s. Check you have sufficient rights.", mpdPath);
	totalDurationInMs += segDurationInMs;
}

void MPEG_DASH::flush() {
	numDataQueueNotify--;
	if ((type == Live) && (numDataQueueNotify == 0))
		endOfStream();
}

}
}
