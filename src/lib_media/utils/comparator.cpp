#include "comparator.hpp"
#include "../common/pcm.hpp"
#include <cmath>


namespace Modules {
namespace Utils {

void IComparator::process(Data data) {
	if (data != nullptr)
		throw error("data not expected");

	for (;;) {
		Data aData, bData;
		auto a = original.tryPop(aData);
		auto b = other.tryPop(bData);
		if (!a || !b) {
			if (a || b)
				throw error("not the same number of samples");
			log(Info, "end of process");
			break;
		}

		auto res = compare(aData, bData);
		if (!res)
			log(Info, "comparison failed");
	}
}

//FIXME: re-implement with multiple inputs
void IComparator::pushOriginal(Data data) {
	original.push(data);
}

void IComparator::pushOther(Data data) {
	other.push(data);
}


PcmComparator::PcmComparator() {
	auto input = addInput(new Input<DataPcm>(this));
	input->setMetadata(new MetadataRawAudio);
}

bool PcmComparator::compare(Data data1, Data data2) const {
	auto pcm1 = safe_cast<const DataPcm>(data1);
	auto pcm2 = safe_cast<const DataPcm>(data2);
	if (pcm1->getFormat() != pcm2->getFormat())
		throw std::runtime_error("Incompatible audio data");

	auto const size1 = pcm1->size();
	auto const size2 = pcm2->size();
	if (size1 != size2)
		Log::msg(Warning, "Sample sizes are different, comparing the overlap.");
	const DataPcm *data;
	if (size1 < size2)
		data = pcm1.get();
	else
		data = pcm2.get();

	for (size_t planeIdx = 0; planeIdx < data->getFormat().numPlanes; ++planeIdx) {
		for (size_t i = 0; i < data->getPlaneSize(planeIdx); ++i) {
			if (abs(pcm1->getPlane(planeIdx)[i] - pcm2->getPlane(planeIdx)[i]) > tolerance) {
				throw std::runtime_error(format("Samples are different at plane %s, index %s.", planeIdx, i));
				return false;
			}
		}
	}

	return true;
}

void PcmComparator::pushOriginal(Data data) {
	IComparator::pushOriginal(data);
}

void PcmComparator::pushOther(Data data) {
	IComparator::pushOther(data);
}

}
}

