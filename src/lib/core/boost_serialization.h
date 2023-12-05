// Copyright 2016-2021:
//   GobySoft, LLC (2013-)
//   Community contributors (see AUTHORS file)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//
//
// This file is part of the Goby Underwater Autonomy Project Libraries
// ("The Goby Libraries").
//
// The Goby Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The Goby Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#ifndef GOBY_MIDDLEWARE_MARSHALLING_BOOST_SERIALIZATION_H
#define GOBY_MIDDLEWARE_MARSHALLING_BOOST_SERIALIZATION_H

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <vector>

#include "goby/middleware/marshalling/interface.h"

#include "netsim/core/common.h"

namespace goby
{
namespace middleware
{

// update when moved into Goby
constexpr int BOOST_SERIALIZATION_SCHEME = 8;

template <typename T, class Enable = void> constexpr const char* boost_serialization_type_name()
{
    return T::goby_boost_serialization_type;
}

template <typename T> struct SerializerParserHelper<T, BOOST_SERIALIZATION_SCHEME>
{
    static std::vector<char> serialize(const T& msg)
    {
        std::vector<char> data;
        boost::iostreams::back_insert_device<std::vector<char>> inserter(data);
        boost::iostreams::stream<boost::iostreams::back_insert_device<std::vector<char>>> stream(
            inserter);
        boost::archive::binary_oarchive oa(stream);

        oa << msg;
        stream.flush();
        return data;
    }

    static std::string type_name(const T& t = T()) { return boost_serialization_type_name<T>(); }

    template <typename CharIterator>
    static std::shared_ptr<T> parse(CharIterator bytes_begin, CharIterator bytes_end,
                                    CharIterator& actual_end, const std::string& type = type_name())
    {
        std::shared_ptr<T> t = std::make_shared<T>();

        boost::iostreams::basic_array_source<char> device(&(*bytes_begin), &(*bytes_end));
        boost::iostreams::stream<boost::iostreams::basic_array_source<char>> stream(device);
        boost::archive::binary_iarchive ia(stream);

        ia >> *t;
        actual_end = bytes_end;

        return t;
    }
};

template <typename T,
          typename std::enable_if<T::goby_boost_serialization_type != nullptr>::type* = nullptr>
constexpr int scheme()
{
    return BOOST_SERIALIZATION_SCHEME;
}

} // namespace middleware
} // namespace goby

// Serialization for TaggedAudioBuffer

namespace boost
{
namespace serialization
{

template <class Archive>
void serialize(Archive& ar, netsim::AudioBuffer& b, const unsigned int version)
{
    ar& b.buffer_start_time;
    ar& b.jack_frame_time;
    ar& b.samples;
}

template <class Archive>
void serialize(Archive& ar, netsim::TaggedAudioBuffer& t, const unsigned int version)
{
    ar& t.buffer;
    ar& t.marker;
    ar& t.packet_id;
}

} // namespace serialization
} // namespace boost

namespace goby
{
namespace middleware
{

template <> constexpr const char* boost_serialization_type_name<netsim::TaggedAudioBuffer>()
{
    return "TaggedAudioBuffer";
}
} // namespace middleware
} // namespace goby

#endif
