#include <fstream>

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/middleware/hdf5_plugin.h"
#include "goby/util/debug_logger.h"
#include "goby/middleware/log.h"

#include "goby/middleware/log/dccl_log_plugin.h"
#include "goby/middleware/log/protobuf_log_plugin.h"

#include "messages/tool.pb.h"

using namespace goby::util::logger;
using goby::glog;

// ensure linking netsim_messages
netsim::protobuf::ToolReceiveStats dummy;



namespace netsim
{
    
    class HDF5Plugin : public goby::middleware::HDF5Plugin
    {
    public:
        HDF5Plugin(goby::middleware::protobuf::HDF5Config* cfg);
    
    
    private:
        bool provide_entry(goby::middleware::HDF5ProtobufEntry* entry) override;
    private:
        std::deque<std::pair<std::string, std::unique_ptr<std::ifstream>>> logs_;
        goby::middleware::log::ProtobufPlugin pb_plugin_;
        goby::middleware::log::ProtobufPlugin dccl_plugin_; 

        std::deque<std::shared_ptr<google::protobuf::Message>> messages_;
        std::string last_channel_;
    };    

}

extern "C"
{
    goby::middleware::HDF5Plugin* goby_hdf5_load(goby::middleware::protobuf::HDF5Config* cfg)
    {
        return new netsim::HDF5Plugin(cfg);
    }
    
}


netsim::HDF5Plugin::HDF5Plugin(goby::middleware::protobuf::HDF5Config* cfg)
    : goby::middleware::HDF5Plugin(cfg)
{
    for(auto file : cfg->input_file())
    {
        std::unique_ptr<std::ifstream> log(new std::ifstream(file.c_str()));
        
        if(!log->is_open())
        {
            glog.is(WARN) && glog << "Could not open " << file << " for reading" << std::endl;
        }
        else
        {
            pb_plugin_.register_read_hooks(*log);
            dccl_plugin_.register_read_hooks(*log);
            
            logs_.push_back(std::make_pair(file, std::move(log)));
        }
        
    }

    glog.is(VERBOSE) && glog << "Processing log: " << logs_.front().first << std::endl;


}

bool netsim::HDF5Plugin::provide_entry(goby::middleware::HDF5ProtobufEntry* entry)
{
    if(!messages_.empty())
    {
        entry->time = 0;
        entry->channel = last_channel_;
        entry->msg = messages_.front();
        messages_.pop_front();
        return true;
    }
    
    while(!logs_.empty())
    {
        auto& current_log = *logs_.front().second;
        try
        {
            goby::middleware::log::LogEntry log_entry;
            log_entry.parse(&current_log);

            if(log_entry.scheme() == goby::middleware::MarshallingScheme::DCCL || log_entry.scheme() == goby::middleware::MarshallingScheme::PROTOBUF)
            {
                if(log_entry.scheme() == goby::middleware::MarshallingScheme::DCCL)
                {
                    auto msgs = dccl_plugin_.parse_message(log_entry);
                    messages_.assign(msgs.begin(), msgs.end());
                }
                else
                {
                    auto msgs = pb_plugin_.parse_message(log_entry);
                    messages_.assign(msgs.begin(), msgs.end());
                }
                
                if(!messages_.empty())
                {
                    //                    std::cout << "#" << messages_.size() << " " << log_entry.type() << " " << log_entry.group() << " " << entry->channel << std::endl;
                    last_channel_ = std::string(log_entry.group());
                    entry->time = 0;
                    entry->channel = last_channel_;
                    entry->msg = messages_.front();
                    messages_.pop_front();
                    return true;
                }
                else
                {
                    glog.is(WARN) && glog << "Protobuf message type " << log_entry.type() << " is not loaded. Skipping." << std::endl;
                }
            }
            else
            {
                glog.is(DEBUG2) && glog << "Skipping scheme: " << log_entry.scheme() << ", group: " << log_entry.group() << ", type: " << log_entry.type() << std::endl;
            }
        }
        catch(std::exception& e)
        {
            if(!current_log.eof())
                glog.is(WARN) && glog << "Error processing log [" << logs_.front().first << "]: " << e.what() << std::endl;

            goby::middleware::log::LogEntry::reset();
            logs_.pop_front();
            if(!logs_.empty())
                glog.is(VERBOSE) && glog << "Processing log: " << logs_.front().first << std::endl;
        }
    }    
    return false;
    
}

