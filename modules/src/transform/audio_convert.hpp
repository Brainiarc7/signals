#pragma once

#include "internal/core/module.hpp"
#include <string>

namespace Modules {
namespace Transform {

class AudioConvert : public Module {
public:
	static AudioConvert* create();
	~AudioConvert();
	void process(std::shared_ptr<Data> data);

private:
	AudioConvert();
};

}
}
