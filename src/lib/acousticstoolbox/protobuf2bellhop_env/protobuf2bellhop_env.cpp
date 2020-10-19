// copyright 2010 t. schneider tes@mit.edu
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.

#include <fstream>
#include <iostream>
#include "environment.h"

#include "goby/version.h"
#if GOBY_VERSION_MAJOR >= 2 
 #include "goby/moos/moos_protobuf_helpers.h"
#else
 #include "goby/moos/libmoos_util/moos_protobuf_helpers.h"
#endif

#include <google/protobuf/io/zero_copy_stream_impl.h>

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::cerr << "Usage xml2bellhop_env.cpp path/to/protobuf_env_file" << std::endl;
        exit(1);
    }
    
    // read in initial env file
    std::ifstream fin;
    fin.open(argv[1], std::ifstream::in);
    if(!fin.is_open())
    {
        std::cerr << "could not open '" << argv[1] << "' for reading." << std::endl;
        exit(1);
    }

    netsim::bellhop::protobuf::Environment env;
    google::protobuf::io::IstreamInputStream is(&fin);
    
    google::protobuf::TextFormat::Parser parser;
    parser.Parse(&is, &env);

    bellhop::Environment::output_env(&std::cout,
				     &std::cout,
				     &std::cout,
				     &std::cout,
				     &std::cout, env);
    
    return 0;
}
