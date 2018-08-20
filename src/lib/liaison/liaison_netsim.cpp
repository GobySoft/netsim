#include <Wt/WFileResource>

#include <Wt/WPanel>
#include <Wt/WComboBox>
#include <Wt/WTable>
#include <Wt/WSpinBox>

#include <boost/filesystem.hpp>

#include "liaison_netsim.h"
#include "iBellhop_messages.pb.h"

using goby::glog;
using namespace Wt;

LiaisonNetsim::LiaisonNetsim(const goby::common::protobuf::LiaisonConfig& cfg)
    : goby::common::LiaisonContainerWithComms<LiaisonNetsim, NetsimCommsThread>(cfg),
    netsim_cfg_(cfg.GetExtension(protobuf::netsim_config)),
    timeseries_box_(new WGroupBox("Audio Timeseries", this)),
    timeseries_image_(new WImage(timeseries_box_)),
    spect_box_(new WGroupBox("Audio Spectrogram", this)),
    spect_image_(new WImage(spect_box_)),
    tl_box_(new WGroupBox("TL / Statistics", this)),
    tl_table_(new WTable(tl_box_)),
    tl_tx_txt_(new WText("Transmitter: ")),
    tl_tx_(new WComboBox),
    tl_rx_txt_(new WText("Receiver: ")),
    tl_rx_(new WComboBox),
    tl_r_txt_(new WText("Range max (meters): ")),
    tl_r_(new WSpinBox),
    tl_dr_txt_(new WText("Delta Range (meters): ")),
    tl_dr_(new WSpinBox),
    tl_z_txt_(new WText("Depth z (meters, negative down): ")),
    tl_z_(new WSpinBox),
    tl_dz_txt_(new WText("Delta Depth (meters): ")),
    tl_dz_(new WSpinBox),
    tl_request_(new WPushButton("Update TL Plot", tl_box_))
{
    tl_table_->elementAt(0, 0)->addWidget(tl_tx_txt_);
    tl_table_->elementAt(0, 1)->addWidget(tl_tx_);

    tl_table_->elementAt(1, 0)->addWidget(tl_rx_txt_);
    tl_table_->elementAt(1, 1)->addWidget(tl_rx_);

    tl_table_->elementAt(2, 0)->addWidget(tl_r_txt_);
    tl_table_->elementAt(2, 1)->addWidget(tl_r_);

    tl_table_->elementAt(3, 0)->addWidget(tl_dr_txt_);
    tl_table_->elementAt(3, 1)->addWidget(tl_dr_);

    tl_table_->elementAt(4, 0)->addWidget(tl_z_txt_);
    tl_table_->elementAt(4, 1)->addWidget(tl_z_);

    tl_table_->elementAt(5, 0)->addWidget(tl_dz_txt_);
    tl_table_->elementAt(5, 1)->addWidget(tl_dz_);

    tl_r_->setRange(0, 100000);
    tl_r_->setSingleStep(1000);
    tl_r_->setValue(10000);

    tl_dr_->setRange(1, 10000);
    tl_dr_->setSingleStep(10);
    tl_dr_->setValue(100);

    tl_z_->setRange(0, 10000);
    tl_z_->setSingleStep(100);
    tl_z_->setValue(3000);
    
    tl_dz_->setRange(1, 1000);
    tl_dz_->setSingleStep(10);
    tl_dz_->setValue(10);

    tl_request_->clicked().connect([this](const WMouseEvent& ev)
				   {
				       iBellhopRequest req;

				       static std::atomic<int> id(2<<16);
				       req.set_request_number(id);				       
				       
				       ++id;
				       
				       auto& env = *req.mutable_env();
				       auto& ai = *env.mutable_adaptive_info();
				       ai.set_contact(tl_rx_->currentText().narrow());
				       ai.set_ownship(tl_tx_->currentText().narrow());

				       // TODO - fix me
				       env.set_freq(4000);
				       auto& output = *env.mutable_output();
				       output.set_type(bellhop::protobuf::Environment::Output::INCOHERENT_PRESSURE);
				       auto& rx = *env.mutable_receivers();
				       auto& tx = *env.mutable_sources();

				       auto z = tl_z_->value();
				       auto dz = tl_dz_->value();
				       auto r = tl_r_->value();
				       auto dr = tl_dr_->value();
				       
				       rx.set_number_in_depth(std::abs(z/dz));
				       rx.set_number_in_range(r/dr);
				       rx.mutable_first()->set_depth(0);
				       rx.mutable_first()->set_range(0);
				       rx.mutable_last()->set_depth(std::abs(z));
				       rx.mutable_last()->set_range(r);

				       tx.set_number_in_depth(1);

				       // TODO - fix me
				       tx.mutable_first()->set_depth(33);

				       auto beams = *env.mutable_beams();
				       beams.set_approximation_type(bellhop::protobuf::Environment::Beams::GAUSSIAN);
				       beams.set_theta_min(-60);
				       beams.set_theta_max(60);
				       beams.set_number(1000);				       
				       
				       this->post_to_comms([=]() { this->goby_thread()->interprocess().publish<groups::bellhop_request>(req); });				       
				   });
    
    set_name("Netsim");
}

void LiaisonNetsim::handle_new_log(const LoggerEvent& event)
{
    if(event.event() == LoggerEvent::ALL_LOGS_CLOSED_FOR_PACKET)
    {
	std::stringstream spect_out_path;
	spect_out_path << event.log_dir() << "/netsim_" << event.start_time() << "_" << std::setw(3) << std::setfill('0') << event.packet_id() << "_spectrogram.png";

	std::stringstream timeseries_out_path;
	timeseries_out_path << event.log_dir() << "/netsim_" << event.start_time() << "_" << std::setw(3) << std::setfill('0') << event.packet_id() << "_timeseries.png";

	{
	    spect_image_resource_.reset(new WFileResource(spect_out_path.str().c_str()));
	    Wt::WLink link(spect_image_resource_.get());
	    spect_image_->setImageLink(link);
	}
	{
	    timeseries_image_resource_.reset(new WFileResource(timeseries_out_path.str().c_str()));
	    Wt::WLink link(timeseries_image_resource_.get());
	    timeseries_image_->setImageLink(link);
	}
    }
}

void LiaisonNetsim::handle_manager_cfg(const NetSimManagerConfig& cfg)
{
    for(const auto& sim_env_pair : cfg.sim_env_pair())
    {
	tl_tx_->addItem(std::to_string(sim_env_pair.modem_tcp_port()));
	tl_rx_->addItem(std::to_string(sim_env_pair.modem_tcp_port()));
    }

    if(tl_rx_->count() > 1)
	tl_rx_->setCurrentIndex(1);

    
}
