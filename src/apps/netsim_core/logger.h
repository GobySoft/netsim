#ifndef LOGGER20171025H
#define LOGGER20171025H

#include <fstream>
#include <sstream>

#include "goby/zeromq/application/multi_thread.h"
#include "netsim/messages/core_config.pb.h"
#include "netsim/messages/logger.pb.h"
#include "netsim/messages/groups.h"

using ThreadBase = goby::middleware::SimpleThread<netsim::protobuf::NetSimCoreConfig>;

class LoggerThread : public ThreadBase
{
private:
    enum class Direction { IN, OUT};

public:
LoggerThread(const netsim::protobuf::NetSimCoreConfig& config)
    : ThreadBase(config, 0)
    {
	
        // subscribe to all the detector output
	for(int i = 0, n = cfg().number_of_modems(); i < n; ++i)
	{
	    auto detector_group_name = std::string("detector_audio_tx_") + std::to_string(i);
	    detector_audio_groups_.push_back(goby::middleware::DynamicGroup(detector_group_name));
	    
	    auto detector_audio_callback = [this, i](std::shared_ptr<const TaggedAudioBuffer> buffer) { this->log_audio(buffer, i, -1, Direction::IN); };
	    interthread().subscribe_dynamic<TaggedAudioBuffer>(detector_audio_callback, detector_audio_groups_[i]);
	}

	// subscribe to all the processor output
       	for(int from_i = 0, n = cfg().number_of_modems(); from_i < n; ++from_i)
	{
	    for(int to_i = 0, m = cfg().number_of_modems(); to_i < m; ++to_i)
	    {
		auto audio_out_group_name = std::string("audio_out_from_") + std::to_string(from_i) + std::string("_to_") + std::to_string(to_i);
		audio_out_groups_.push_back(goby::middleware::DynamicGroup(audio_out_group_name));
		
		auto audio_out_callback = [this, from_i, to_i](std::shared_ptr<const TaggedAudioBuffer> buffer) { this->log_audio(buffer, from_i, to_i, Direction::OUT); };
		interthread().subscribe_dynamic<TaggedAudioBuffer>(audio_out_callback, audio_out_groups_[from_i*cfg().number_of_modems()+to_i]);
	    }
	}
    }

private:

    void log_audio(std::shared_ptr<const TaggedAudioBuffer> buffer, int from_modem_index, int to_modem_index, Direction dir)
    {
	using goby::glog; using namespace goby::util::logger;

	int modem_index = (dir == Direction::IN) ? from_modem_index : to_modem_index;	    
       
	if(buffer->marker == TaggedAudioBuffer::Marker::START)
	{
	    std::stringstream file_name;
	    file_name << cfg().logger().log_directory() << "/netsim_" << start_time << "_"
		      << ((dir == Direction::IN) ? "in_" : "out_")
		      << std::setw(3) << std::setfill('0') << buffer->packet_id
		      << "_modem" << std::to_string(modem_index) << ".bin";

	    files_[dir][buffer->packet_id][modem_index].reset(new std::ofstream(file_name.str().c_str(),
										std::ios::out | std::ios::binary));

	    // write double time to the start of the file
	    auto& file_ptr = files_[dir][buffer->packet_id][modem_index];
	    file_ptr->write(reinterpret_cast<const char*>(&buffer->buffer->buffer_start_time), sizeof(double));

	    if(dir == Direction::IN)
	    {
		netsim::protobuf::LoggerEvent event;
		event.set_event(netsim::protobuf::LoggerEvent::PACKET_START);
		event.set_packet_id(buffer->packet_id);
		event.set_tx_modem_id(modem_index);
		interprocess().publish<netsim::groups::logger_event>(event);
	    }
	    
	}

	auto& file_ptr = files_[dir][buffer->packet_id][modem_index];
	if(file_ptr)
	    file_ptr->write(reinterpret_cast<const char*>(&buffer->buffer->samples[0]), buffer->buffer->samples.size()*sizeof(sample_t));
//	else
//	    glog.is(WARN) && glog << "No TaggedAudioBuffer::Marker::START so cannot log to file. Modem " << modem_index << ", dir: " << dir_to_str(dir)  << std::endl;	   

	// cleanly close out file
	if(buffer->marker == TaggedAudioBuffer::Marker::END)
	{
	    files_[dir][buffer->packet_id].erase(modem_index);

	    if(dir == Direction::OUT && files_[Direction::OUT][buffer->packet_id].empty())
	    {
		netsim::protobuf::LoggerEvent event;
		event.set_event(netsim::protobuf::LoggerEvent::ALL_LOGS_CLOSED_FOR_PACKET);
		event.set_log_dir(cfg().logger().log_directory());
		std::stringstream ss_time;
		ss_time << start_time;
		event.set_start_time(ss_time.str());
		event.set_packet_id(buffer->packet_id);
		interprocess().publish<netsim::groups::logger_event>(event);
	    }
	}
    }


    std::string dir_to_str(Direction dir)
    { return (dir == Direction::IN) ? "IN": "OUT"; }
    
private:
    std::vector<goby::middleware::DynamicGroup> detector_audio_groups_;
    std::vector<goby::middleware::DynamicGroup> audio_out_groups_;

    // map direction to packet_id to input/output modem_id
    std::map<Direction, std::map<int, std::map<int, std::unique_ptr<std::ofstream>>>> files_;
    std::string start_time{goby::time::file_str()};
};

#endif
