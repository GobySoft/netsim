#ifndef MODEMSIMGROUPS20170815H
#define MODEMSIMGROUPS20170815H

#include "goby/middleware/group.h"

namespace groups
{    
    constexpr goby::Group impulse_request{"impulse_request"};
    constexpr goby::Group impulse_response{"impulse_response"};
    constexpr goby::Group buffer_size_change{"buffer_size_change"};
}

#endif
