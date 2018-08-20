#include <Wt/WFileResource>

#include <Wt/WPanel>
#include <Wt/WComboBox>

#include <boost/filesystem.hpp>

#include "liaison_netsim.h"

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
    tl_tx_(new WComboBox(tl_box_)),
    tl_rx_(new WComboBox(tl_box_)),
    tl_r_(new WLineEdit(tl_box_)),
    tl_dr_(new WLineEdit(tl_box_)),
    tl_z_(new WLineEdit(tl_box_)),
    tl_dz_(new WLineEdit(tl_box_)),
    tl_request_(new WPushButton("Update TL Plot", tl_box_))
{
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

	static std::atomic<int> octave_run {-1};
	
	if(octave_run != event.packet_id())
	{
	    octave_run = event.packet_id();
	    std::stringstream octave_cmd;
	    octave_cmd << "flatpak run org.octave.Octave /opt/netsim/src/octave/liaison_plot_signal.m " << event.log_dir() << " " << event.start_time() << " " << event.packet_id();
	    glog.is_debug1() && glog << "Running: " << octave_cmd.str() << std::endl;
	    system(octave_cmd.str().c_str());
	    glog.is_debug1() && glog << "Octave complete" << std::endl;
	}
	else
	{
	    while(!boost::filesystem::exists(spect_out_path.str()))
		usleep(10000);

	}

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
