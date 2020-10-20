// Copyright 2017-2020:
//   GobySoft, LLC (2017-)
//   Massachusetts Institute of Technology (2017-)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//   Henrik Schmidt <henrik@mit.edu>
//
//
// This file is part of the NETSIM Libraries.
//
// The NETSIM Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The NETSIM Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with NETSIM.  If not, see <http://www.gnu.org/licenses/>.

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/moos/middleware/moos_plugin_translator.h"
#include "goby/moos/moos_translator.h"

#include "netsim/messages/env_bellhop_req.pb.h"
#include "netsim/messages/groups.h"
#include "netsim/messages/netsim.pb.h"

using goby::glog;
using namespace goby::util::logger;
using goby::apps::moos::protobuf::GobyMOOSGatewayConfig;

namespace netsim
{
class NetSimCoreTranslation : public goby::moos::Translator
{
  public:
    using Base = goby::moos::Translator;

    NetSimCoreTranslation(const GobyMOOSGatewayConfig& cfg)
        : Base(cfg), environment_id_(cfg.moos().port() % 10) // based on MOOS port 0-9
    {
        glog.is(DEBUG1) && glog << "Environment id: " << environment_id_ << std::endl;

        goby()
            .interprocess()
            .subscribe<netsim::groups::env_impulse_req,
                       netsim::protobuf::EnvironmentImpulseRequest>(
                [this](const netsim::protobuf::EnvironmentImpulseRequest& i) {
                    this->goby_to_moos(i);
                });

        goby()
            .interprocess()
            .subscribe<netsim::groups::env_nav_update, netsim::protobuf::EnvironmentNavUpdate>(
                [this](const netsim::protobuf::EnvironmentNavUpdate& n) { this->goby_to_moos(n); });

        goby()
            .interprocess()
            .subscribe<netsim::groups::env_bellhop_req,
                       netsim::protobuf::EnvironmentiBellhopRequest>(
                [this](const netsim::protobuf::EnvironmentiBellhopRequest& n) {
                    this->goby_to_moos(n);
                });

        goby()
            .interprocess()
            .subscribe<netsim::groups::env_performance_req,
                       netsim::protobuf::EnvironmentObjFuncRequest>(
                [this](const netsim::protobuf::EnvironmentObjFuncRequest& i) {
                    this->goby_to_moos(i);
                });

        moos().add_trigger(imp_resp_var_, [this](const CMOOSMsg& msg) { moos_to_goby(msg); });
        moos().add_trigger(bellhop_resp_var_, [this](const CMOOSMsg& msg) { moos_to_goby(msg); });
        moos().add_trigger(perf_resp_var_, [this](const CMOOSMsg& msg) { moos_to_goby(msg); });

        {
            goby::moos::protobuf::TranslatorEntry imp_resp_entry;
            imp_resp_entry.set_protobuf_name(
                netsim::protobuf::ImpulseResponse::descriptor()->full_name());
            auto& create = *imp_resp_entry.add_create();
            create.set_technique(
                goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
            create.set_moos_var(imp_resp_var_);
            translator_.add_entry(imp_resp_entry);
        }

        {
            goby::moos::protobuf::TranslatorEntry imp_req_entry;
            imp_req_entry.set_protobuf_name(
                netsim::protobuf::ImpulseRequest::descriptor()->full_name());
            auto& publish = *imp_req_entry.add_publish();
            publish.set_technique(
                goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
            publish.set_moos_var(imp_req_var_);
            translator_.add_entry(imp_req_entry);
        }

        {
            goby::moos::protobuf::TranslatorEntry nav_update_entry;
            nav_update_entry.set_protobuf_name(
                netsim::protobuf::NavUpdate::descriptor()->full_name());
            auto& publish = *nav_update_entry.add_publish();
            publish.set_technique(
                goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
            publish.set_moos_var(nav_update_var_);
            translator_.add_entry(nav_update_entry);
        }

        {
            goby::moos::protobuf::TranslatorEntry bellhop_resp_entry;
            bellhop_resp_entry.set_protobuf_name(netsim::protobuf::iBellhopResponse::descriptor()->full_name());
            auto& create = *bellhop_resp_entry.add_create();
            create.set_technique(
                goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
            create.set_moos_var(bellhop_resp_var_);
            translator_.add_entry(bellhop_resp_entry);
        }

        {
            goby::moos::protobuf::TranslatorEntry bellhop_req_entry;
            bellhop_req_entry.set_protobuf_name(netsim::protobuf::iBellhopRequest::descriptor()->full_name());
            auto& publish = *bellhop_req_entry.add_publish();
            publish.set_technique(
                goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
            publish.set_moos_var(bellhop_req_var_);
            translator_.add_entry(bellhop_req_entry);
        }
        // HS 2020-05-08 >>>>>>>
        {
            goby::moos::protobuf::TranslatorEntry perf_resp_entry;
            perf_resp_entry.set_protobuf_name(
                netsim::protobuf::ObjFuncResponse::descriptor()->full_name());
            auto& create = *perf_resp_entry.add_create();
            create.set_technique(
                goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
            create.set_moos_var(perf_resp_var_);
            translator_.add_entry(perf_resp_entry);
        }

        {
            goby::moos::protobuf::TranslatorEntry perf_req_entry;
            perf_req_entry.set_protobuf_name(
                netsim::protobuf::ObjFuncRequest::descriptor()->full_name());
            auto& publish = *perf_req_entry.add_publish();
            publish.set_technique(
                goby::moos::protobuf::TranslatorEntry::TECHNIQUE_PREFIXED_PROTOBUF_TEXT_FORMAT);
            publish.set_moos_var(perf_req_var_);
            translator_.add_entry(perf_req_entry);
        }
        // <<<<<<
    }

  private:
    void moos_to_goby(const CMOOSMsg& moos_msg)
    {
        if (moos_msg.GetKey() == imp_resp_var_)
        {
            // publish IMPULSE_RESPONSE
            std::map<std::string, CMOOSMsg> moos_msgs = {{moos_msg.GetKey(), moos_msg}};
            auto imp_resp_pb =
                translator_.moos_to_protobuf<std::shared_ptr<google::protobuf::Message>>(
                    moos_msgs, "netsim.protobuf.ImpulseResponse");
            goby().interprocess().publish<netsim::groups::impulse_response>(
                std::dynamic_pointer_cast<netsim::protobuf::ImpulseResponse>(imp_resp_pb));
        }
        else if (moos_msg.GetKey() == bellhop_resp_var_)
        {
            std::map<std::string, CMOOSMsg> moos_msgs = {{moos_msg.GetKey(), moos_msg}};
            auto bellhop_resp_pb =
                translator_.moos_to_protobuf<std::shared_ptr<google::protobuf::Message>>(
                    moos_msgs, "netsim.protobuf.iBellhopResponse");
            goby().interprocess().publish<netsim::groups::bellhop_response>(
                std::dynamic_pointer_cast<netsim::protobuf::iBellhopResponse>(bellhop_resp_pb));
        }
        else if (moos_msg.GetKey() == perf_resp_var_)
        {
            std::map<std::string, CMOOSMsg> moos_msgs = {{moos_msg.GetKey(), moos_msg}};
            auto perf_resp_pb =
                translator_.moos_to_protobuf<std::shared_ptr<google::protobuf::Message>>(
                    moos_msgs, "netsim.protobuf.ObjFuncResponse");
            goby().interprocess().publish<netsim::groups::performance_response>(
                std::dynamic_pointer_cast<netsim::protobuf::ObjFuncResponse>(perf_resp_pb));
        }
    }

    void goby_to_moos(const netsim::protobuf::EnvironmentImpulseRequest& req)
    {
        if (req.environment_id() == environment_id_)
        {
            std::multimap<std::string, CMOOSMsg> moos_msgs =
                translator_.protobuf_to_moos(req.req());
            for (auto& moos_msg_pair : moos_msgs) moos().comms().Post(moos_msg_pair.second);
        }
    }

    void goby_to_moos(const netsim::protobuf::EnvironmentNavUpdate& update)
    {
        if (update.environment_id() == environment_id_)
        {
            std::multimap<std::string, CMOOSMsg> moos_msgs =
                translator_.protobuf_to_moos(update.nav());
            for (auto& moos_msg_pair : moos_msgs) moos().comms().Post(moos_msg_pair.second);
        }
    }

    void goby_to_moos(const netsim::protobuf::EnvironmentiBellhopRequest& req)
    {
        if (req.environment_id() == environment_id_)
        {
            std::multimap<std::string, CMOOSMsg> moos_msgs =
                translator_.protobuf_to_moos(req.req());
            for (auto& moos_msg_pair : moos_msgs) moos().comms().Post(moos_msg_pair.second);
        }
    }

    void goby_to_moos(const netsim::protobuf::EnvironmentObjFuncRequest& req)
    {
        if (req.environment_id() == environment_id_)
        {
            std::multimap<std::string, CMOOSMsg> moos_msgs =
                translator_.protobuf_to_moos(req.req());
            for (auto& moos_msg_pair : moos_msgs) moos().comms().Post(moos_msg_pair.second);
        }
    }

  private:
    goby::moos::MOOSTranslator translator_;
    const std::string imp_resp_var_{"IMPULSE_RESPONSE"};
    const std::string imp_req_var_{"IMPULSE_REQUEST"};
    const std::string nav_update_var_{"NETSIM_NAV_UPDATE"};

    const std::string bellhop_req_var_{"BELLHOP_REQUEST"};
    const std::string bellhop_resp_var_{"BELLHOP_RESPONSE"};
    // HS 2020-05-08 - performance estimate
    const std::string perf_resp_var_{"PERFORMANCE_RESPONSE"};
    const std::string perf_req_var_{"PERFORMANCE_REQUEST"};

    int environment_id_;
};
} // namespace netsim

extern "C"
{
    void
    goby3_moos_gateway_load(goby::zeromq::MultiThreadApplication<GobyMOOSGatewayConfig>* handler)
    {
        handler->launch_thread<netsim::NetSimCoreTranslation>();
    }

    void
    goby3_moos_gateway_unload(goby::zeromq::MultiThreadApplication<GobyMOOSGatewayConfig>* handler)
    {
        handler->join_thread<netsim::NetSimCoreTranslation>();
    }
}
