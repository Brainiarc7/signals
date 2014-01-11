#include "gpac_mp4_simple.hpp"
#include "internal/log.hpp"
#include <string>

extern "C" {
#include <gpac/tools.h>
#include <gpac/isomedia.h>
}

#include "gpacpp.h"

class ISOFileReader {
public:
  std::unique_ptr<gpacpp::IsoFile> movie;
	uint32_t track_number;
  std::unique_ptr<gpacpp::IsoSample> iso_sample;
	uint32_t sample_index, sample_count;
};


GPAC_MP4_Simple* GPAC_MP4_Simple::create(const Param &parameters) {
	auto filename = parameters.find("filename");
	if (filename == parameters.end()) {
		return NULL;
	}
	const std::string &fn = (*filename).second;

	/* The ISO progressive reader */
	GF_ISOFile *movie;
	/* Number of bytes required to finish the current ISO Box reading */
	u64 missing_bytes;

	GF_Err e = gf_isom_open_progressive(fn.c_str(), 0, 0, &movie, &missing_bytes);
	if ((e != GF_OK && e != GF_ISOM_INCOMPLETE_FILE) || movie == NULL) {
		Log::get(Log::Error) << "Could not open file " << fn << " for reading (" << gf_error_to_string(e) << ")." << std::endl;
		return NULL;
	}

	return new GPAC_MP4_Simple(movie);
}

void GPAC_MP4_Simple::deleteLastSample() {
  reader->iso_sample.reset();
}

GPAC_MP4_Simple::GPAC_MP4_Simple(GF_ISOFile *movie)
: reader(new ISOFileReader) {
  reader->movie.reset(new gpacpp::IsoFile(movie));
	gf_sys_init(GF_FALSE);
	u32 track_id = gf_isom_get_track_id(movie, 1); //FIXME should be a parameter? hence not processed in create() but in a stateful process? or a control module?
	reader->track_number = gf_isom_get_track_by_id(movie, track_id);
	if (reader->track_number == 0) {
		Log::get(Log::Error) << "Could not find track ID=" << track_id << std::endl;
	}
	reader->sample_count = gf_isom_get_sample_count(movie, reader->track_number);
	reader->sample_index = 1;
	signals.push_back(new Pin);
}

GPAC_MP4_Simple::~GPAC_MP4_Simple() {
	delete signals[0];
	gf_sys_close();
}

bool GPAC_MP4_Simple::process(std::shared_ptr<Data> /*data*/) {
  try {
    int sample_description_index;
    reader->iso_sample.reset();
    reader->iso_sample = reader->movie->getSample(reader->track_number, reader->sample_index, sample_description_index);

		Log::get(Log::Error) << "Found sample #" << reader->sample_index << "/" << reader->sample_count << " of length " << reader->iso_sample->dataLength << ", RAP: " << reader->iso_sample->IsRAP << ", DTS: " << reader->iso_sample->DTS << ", CTS: " << reader->iso_sample->DTS + reader->iso_sample->CTS_Offset << std::endl;
		reader->sample_index++;

		std::shared_ptr<Data> out(new Data(reader->iso_sample->dataLength));
		memcpy(out->data(), reader->iso_sample->data, reader->iso_sample->dataLength);
		signals[0]->emit(out);

		/*release the sample data, once you're done with it*/
    reader->iso_sample.reset();
  }
  catch(gpacpp::Error const& err) {
		if (err.error_ == GF_ISOM_INCOMPLETE_FILE) {
			u64 missing_bytes = reader->movie->getMissingBytes(reader->track_number);
			Log::get(Log::Error) << "Missing " << missing_bytes << " bytes on input file" << std::endl;
		} else {
			return false;
		}
	}

	return true;
}

bool GPAC_MP4_Simple::handles(const std::string &url) {
	return GPAC_MP4_Simple::canHandle(url);
}

bool GPAC_MP4_Simple::canHandle(const std::string &url) {
	if (url.find_last_of("mp4") + 1 == url.size()) {
		return true;
	} else {
		return false;
	}
}
