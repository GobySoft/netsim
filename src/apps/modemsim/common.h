#ifndef COMMON20170816H
#define COMMON20170816H

#include <jack/jack.h>

#include <vector>


typedef float sample_t;

struct AudioBuffer
{
AudioBuffer(size_t size) : samples(size, 0) {}    

    template<typename It>
      AudioBuffer(It begin, It end) : samples(begin, end) {}


    std::vector<sample_t> samples;
    double buffer_start_time{0};
    
    enum class Marker
    { NONE, START, END };
    Marker marker{Marker::NONE};
    int packet_id;
};










#endif

