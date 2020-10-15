#ifndef COMMON20170816H
#define COMMON20170816H

#include <jack/jack.h>

#include <vector>

namespace netsim
{

typedef float sample_t;

struct AudioBuffer
{
    AudioBuffer(size_t size) : samples(size, 0) {}    

    template<typename It>
    AudioBuffer(It begin, It end) : samples(begin, end) {}

    double buffer_start_time{0};
    jack_nframes_t jack_frame_time{0};
    
    std::vector<netsim::sample_t> samples;
};

struct TaggedAudioBuffer
{
    std::shared_ptr<const AudioBuffer> buffer;
    
    enum class Marker
    { NONE, START, END, MIDDLE };
    Marker marker{Marker::NONE};
    int packet_id { -1 };
};

}

#endif

