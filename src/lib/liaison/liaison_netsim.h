// Copyright 2018-2020:
//   GobySoft, LLC (2017-)
//   Massachusetts Institute of Technology (2017-)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
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

#ifndef LIAISONNETSIM20180820H
#define LIAISONNETSIM20180820H

#include <Wt/WBorder.h>
#include <Wt/WColor.h>
#include <Wt/WCssDecorationStyle.h>
#include <Wt/WDoubleSpinBox.h>
#include <Wt/WGroupBox.h>
#include <Wt/WImage.h>
#include <Wt/WPaintDevice.h>
#include <Wt/WPaintedWidget.h>
#include <Wt/WPainter.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>
#include <Wt/WVBoxLayout.h>

#include "goby/middleware/marshalling/protobuf.h"

#include "goby/zeromq/application/multi_thread.h"
#include "goby/zeromq/liaison/liaison_container.h"

#include "netsim/messages/config_request.pb.h"
#include "netsim/messages/groups.h"
#include "netsim/messages/liaison.pb.h"
#include "netsim/messages/logger.pb.h"
#include "netsim/messages/manager_config.pb.h"
#include "netsim/messages/netsim.pb.h"

#include "netsim/acousticstoolbox/iBellhop_messages.pb.h"

namespace netsim
{
class NetsimCommsThread;
class LiaisonNetsim;

class TLPaintedWidget : public Wt::WPaintedWidget
{
  public:
    TLPaintedWidget(LiaisonNetsim* netsim) : netsim_(netsim) { resize(10, 10); }

  protected:
    void paintEvent(Wt::WPaintDevice* paintDevice);

  private:
    LiaisonNetsim* netsim_;
    int text_length_{50};
    int diam_{8};
    // modem_tcp_ports that have reset their key legend
    //    std::set<int> refresh_key_;
};

class LiaisonNetsim
    : public goby::zeromq::LiaisonContainerWithComms<LiaisonNetsim, NetsimCommsThread>
{
  public:
    LiaisonNetsim(const goby::apps::zeromq::protobuf::LiaisonConfig& cfg);

    void handle_new_log(const netsim::protobuf::LoggerEvent& event);
    void handle_manager_cfg(const netsim::protobuf::NetSimManagerConfig& cfg);
    void handle_bellhop_resp(const netsim::protobuf::iBellhopResponse& resp);

    void handle_updated_nav(const netsim::protobuf::EnvironmentNavUpdate& update)
    {
        last_nav_[update.nav().modem_tcp_port()] = update.nav();
    }
    void handle_receive_stats(const netsim::protobuf::ReceiveStats& stats)
    {
        receive_stats_[stats.tx_modem_tcp_port()][stats.modem_tcp_port()] = stats;
    }

  private:
    friend class TLPaintedWidget;

    const netsim::protobuf::LiaisonNetsimConfig& netsim_cfg_;

    Wt::WPanel* timeseries_panel_;
    Wt::WImage* timeseries_image_;
    std::shared_ptr<Wt::WResource> timeseries_image_resource_;
    Wt::WPanel* spect_panel_;
    Wt::WImage* spect_image_;
    std::shared_ptr<Wt::WResource> spect_image_resource_;

    Wt::WPanel* tl_panel_;
    TLPaintedWidget* tl_plot_;
    std::shared_ptr<Wt::WResource> tl_plot_resource_;

    Wt::WTable* tl_table_;

    Wt::WComboBox* tl_tx_;
    Wt::WSpinBox* tl_r_;
    Wt::WSpinBox* tl_dr_;
    Wt::WSpinBox* tl_z_;
    Wt::WSpinBox* tl_dz_;

    Wt::WPushButton* tl_request_;

    int transmitter_tcp_port_{0};
    int r_, dr_, z_, dz_;

    std::string tl_image_path_;

    netsim::protobuf::iBellhopResponse tl_bellhop_resp_;

    std::vector<Wt::WPen> rx_pens_{Wt::WPen(Wt::StandardColor::White),  Wt::WPen(Wt::StandardColor::White), Wt::WPen(Wt::StandardColor::Yellow),
                                   Wt::WPen(Wt::StandardColor::Yellow), Wt::WPen(Wt::StandardColor::Gray),  Wt::WPen(Wt::StandardColor::Gray)};

    // int = modem_tcp_port
    std::map<int, netsim::protobuf::NavUpdate> last_nav_;

    // tx modem_tcp_port -> map of nav (last tx)
    std::map<int, std::map<int, netsim::protobuf::NavUpdate>> nav_at_last_tx_start_;

    // tx modem_tcp_port -> map of nav (one prior to last tx)
    std::map<int, std::map<int, netsim::protobuf::NavUpdate>> nav_at_previous_tx_start_;

    // tx_modem_tcp_port -> rx_modem_tcp_port -> netsim::protobuf::ReceiveStats
    std::map<int, std::map<int, netsim::protobuf::ReceiveStats>> receive_stats_;

    netsim::protobuf::NetSimManagerConfig manager_cfg_;
};

class NetsimCommsThread : public goby::zeromq::LiaisonCommsThread<LiaisonNetsim>
{
  public:
    NetsimCommsThread(LiaisonNetsim* wt_app,
                      const goby::apps::zeromq::protobuf::LiaisonConfig& config, int index)
        : goby::zeromq::LiaisonCommsThread<LiaisonNetsim>(wt_app, config, index), wt_app_(wt_app)
    {
        interprocess().subscribe<netsim::groups::post_process_event, netsim::protobuf::LoggerEvent>(
            [this](const netsim::protobuf::LoggerEvent& event)
            { wt_app_->post_to_wt([=]() { wt_app_->handle_new_log(event); }); });

        interprocess()
            .subscribe<netsim::groups::post_process_event, netsim::protobuf::iBellhopResponse>(
                [this](const netsim::protobuf::iBellhopResponse& resp)
                { wt_app_->post_to_wt([=]() { wt_app_->handle_bellhop_resp(resp); }); });

        interprocess()
            .subscribe<netsim::groups::configuration, netsim::protobuf::NetSimManagerConfig>(
                [this](const netsim::protobuf::NetSimManagerConfig& manager_cfg)
                { wt_app_->post_to_wt([=]() { wt_app_->handle_manager_cfg(manager_cfg); }); });

        interprocess()
            .subscribe<netsim::groups::env_nav_update, netsim::protobuf::EnvironmentNavUpdate>(
                [this](const netsim::protobuf::EnvironmentNavUpdate& update)
                { wt_app_->post_to_wt([=]() { wt_app_->handle_updated_nav(update); }); });

        interprocess().subscribe<netsim::groups::receive_stats, netsim::protobuf::ReceiveStats>(
            [this](const netsim::protobuf::ReceiveStats& stats)
            { wt_app_->post_to_wt([=]() { wt_app_->handle_receive_stats(stats); }); });

        netsim::protobuf::ConfigRequest req;
        req.set_subsystem(netsim::protobuf::ConfigRequest::MANAGER);
        interprocess().publish<netsim::groups::config_request>(req);
    }
    ~NetsimCommsThread() {}

  private:
    friend class LiaisonNetsim;
    LiaisonNetsim* wt_app_;
};
} // namespace netsim

#endif
