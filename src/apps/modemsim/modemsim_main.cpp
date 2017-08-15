//#include <boost/circular_buffer.hpp>
//#include <fstream>

#include "goby/middleware/multi-thread-application.h"

#include "config.pb.h"
#include "groups.h"
#include "jack_thread.h"

class ModemSim : public goby::MultiThreadApplication<ModemSimConfig>
{
public:
    ModemSim()
        {            
            launch_thread<JackThread>();
            //launch_thread<BasicSubscriber>(0);
            //launch_thread<BasicSubscriber>(1);
        }
};



int main(int argc, char* argv[])
{ return goby::run<ModemSim>(argc, argv); }


// class PacketDetector
// {
// public:
//     PacketDetector(const std::string& file):
// 	file_(file)
// 	{
// 	}

//     ~PacketDetector()
// 	{ }
	          
//     bool ready() { return have_start && have_end; }
//     void write_file()
// 	{
// 	    if(!ready())
// 	    {
// 		std::cerr << "Cannot write file, not ready." << std::endl;
// 		return;
// 	    }
// 	    std::cout << "Writing file of " << static_cast<float>(packet_end->first - packet_start->first)/static_cast<float>(fs) << " seconds" << std::endl;
// 	    // write file
// 	    std::ofstream ofs(file_.c_str(), std::ios::out | std::ios::binary);

// 	    for(auto it = packet_start - buffer_size/2; it != packet_end + buffer_size/2; ++it)
// 		ofs.write((char*)&it->second, sizeof(float));
// 	    std::cout << "Write complete" << std::endl;
// 	}

//     void copy_new_frames(typename decltype(rx_buffer)::const_iterator begin,
// 			 typename decltype(rx_buffer)::const_iterator end)
// 	{
// 	    bool buffer_was_empty = buffer_.empty();
	    
// 	    for(auto it = begin; it != end; ++it)
// 		buffer_.push_back(*it);

// 	    new_nframes_ += (end - begin);

// 	    // set packet_start to a valid iterator
// 	    if(buffer_was_empty)
// 		packet_start = buffer_.begin();
// 	}
    
//     void check_new_nframes()
// 	{
// 	    if(!have_start)
// 	    {
// 		// -1 so we don't invalidate packet_start by letting it go to buffer_.end()
// 		for(auto end = packet_start + new_nframes_ - 1; packet_start != end; ++packet_start)
// 		{
// 		    if(std::abs(packet_start->second) >= detection_threshold)
// 		    {
// 			have_start = true;
// 			potential_packet_end = packet_start;
// 			packet_end = packet_start;
// 			std::cout << "Detected start at: " << packet_start->first << std::endl;
// 			break;
// 		    }
// 		}
// 	    }
// 	    else if(!have_end)
// 	    {
// 		// potential_packet_end tracks the end of the available buffer
// 		// packet_end is dropped off as soon as the threshold is met
// 		for(auto end = potential_packet_end + new_nframes_ - 1; potential_packet_end != end; ++potential_packet_end)
// 		{
// 		    if(std::abs(potential_packet_end->second) < detection_threshold)
// 		    {
// 			if((potential_packet_end->first - packet_end->first) >= quiet_frames_after_packet)
// 			{
// 			    have_end = true;
// 			    std::cout << "Detected end at: " << packet_end->first << std::endl;
// 			    break;
// 			}
// 		    }
// 		    else
// 		    {
// 			packet_end = potential_packet_end;
// 		    }
// 		}
// 	    }
// 	    new_nframes_ = 0;
// 	}
    
// private:
//     boost::circular_buffer<std::pair<jack_nframes_t, sample_t>> buffer_{fs*buffer_seconds};
//     typename boost::circular_buffer<std::pair<jack_nframes_t, sample_t>>::const_iterator packet_start;
//     typename boost::circular_buffer<std::pair<jack_nframes_t, sample_t>>::const_iterator potential_packet_end;
//     typename boost::circular_buffer<std::pair<jack_nframes_t, sample_t>>::const_iterator packet_end;
//     jack_nframes_t new_nframes_{0};
//     bool have_start{false}, have_end{false};
//     const std::string& file_;
// };


    /* keep running until stopped by the user */

    // std::unique_ptr<PacketDetector> packet_detector;
    // while(1)
    // {
    // 	if(!packet_detector)
    // 	    packet_detector.reset(new PacketDetector("/tmp/test.dat"));
    // 	{
    // 	    std::unique_lock<std::mutex> lock(rx_buffer_mutex);
    // 	    rx_buffer_cv.wait(lock, []{return data_ready;});
    // 	    static int i = 0;
    // 	    packet_detector->copy_new_frames(rx_buffer.begin(), rx_buffer.end());
    // 	    data_ready = false;
    // 	}
	
    // 	packet_detector->check_new_nframes();
    // 	if(packet_detector->ready())
    // 	{
    // 	    packet_detector->write_file();
    // 	    packet_detector.reset();
    // 	}
    // }

