#include <fstream>

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/middleware/hdf5_plugin.h"
#include "goby/util/debug_logger.h"
#include "goby/middleware/log.h"

using namespace goby::util::logger;
using goby::glog;

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
            glog.is(WARN) && glog << "Could not open " << file << " for reading" << std::endl;
        else
            logs_.push_back(std::make_pair(file, std::move(log)));
    }

    glog.is(VERBOSE) && glog << "Processing log: " << logs_.front().first << std::endl;
}

bool netsim::HDF5Plugin::provide_entry(goby::middleware::HDF5ProtobufEntry* entry)
{
    while(!logs_.empty())
    {
        auto& current_log = *logs_.front().second;
        try
        {
            goby::middleware::log::LogEntry log_entry;
            log_entry.parse(&current_log);

            if(log_entry.scheme() == goby::middleware::MarshallingScheme::DCCL || log_entry.scheme() == goby::middleware::MarshallingScheme::PROTOBUF)
            {
                auto msg = dccl::DynamicProtobufManager::new_protobuf_message<std::unique_ptr<google::protobuf::Message>>(log_entry.type());
                msg->ParseFromArray(&log_entry.data()[0], log_entry.data().size());
                if(msg)
                {
                    entry->channel = std::string(log_entry.group());
                    std::cout << log_entry.type() << " " << log_entry.group() << " " << entry->channel << std::endl;

                    auto desc = msg->GetDescriptor();
                    auto refl = msg->GetReflection();
                    entry->time = 0;
                    
                    entry->msg = std::shared_ptr<google::protobuf::Message>(std::move(msg));
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

