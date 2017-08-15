#ifndef MODEMSIMGROUPS20170815H
#define MODEMSIMGROUPS20170815H

#include "goby/middleware/group.h"

namespace groups
{    
    constexpr goby::Group audio_in{"audio_in"};
    constexpr goby::Group audio_out{"audio_out"};
    constexpr goby::Group detector_data{"detector_data"};
}

#endif
