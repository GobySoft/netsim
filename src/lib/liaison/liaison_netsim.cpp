#include <Wt/WFileResource>

#include <Wt/WPanel>
#include <Wt/WComboBox>
#include <Wt/WTable>
#include <Wt/WSpinBox>

#include <boost/filesystem.hpp>

#include "liaison_netsim.h"
#include "iBellhop_messages.pb.h"

#include <goby/util/geodesy.h>

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
    tl_plot_(new TLPaintedWidget(this, tl_box_)),
    tl_table_(new WTable(tl_box_)),
    tl_tx_txt_(new WText("Transmitter: ")),
    tl_tx_(new WComboBox),
//    tl_rx_txt_(new WText("Receiver: ")),
//    tl_rx_(new WComboBox),
    tl_r_txt_(new WText("Range max (meters): ")),
    tl_r_(new WSpinBox),
    tl_dr_txt_(new WText("Delta Range (meters): ")),
    tl_dr_(new WSpinBox),
    tl_z_txt_(new WText("Depth z (meters, negative down): ")),
    tl_z_(new WSpinBox),
    tl_dz_txt_(new WText("Delta Depth (meters): ")),
    tl_dz_(new WSpinBox),
    tl_request_(new WPushButton("Update / Clear TL Plot", tl_box_))
{
    for(auto& rx_pen : rx_pens_)
	rx_pen.setWidth(1);     

    tl_table_->elementAt(0, 0)->addWidget(tl_tx_txt_);
    tl_table_->elementAt(0, 1)->addWidget(tl_tx_);

//    tl_table_->elementAt(1, 0)->addWidget(tl_rx_txt_);
//    tl_table_->elementAt(1, 1)->addWidget(tl_rx_);

    tl_table_->elementAt(1, 0)->addWidget(tl_r_txt_);
    tl_table_->elementAt(1, 1)->addWidget(tl_r_);

    tl_table_->elementAt(2, 0)->addWidget(tl_dr_txt_);
    tl_table_->elementAt(2, 1)->addWidget(tl_dr_);

    tl_table_->elementAt(3, 0)->addWidget(tl_z_txt_);
    tl_table_->elementAt(3, 1)->addWidget(tl_z_);

    tl_table_->elementAt(4, 0)->addWidget(tl_dz_txt_);
    tl_table_->elementAt(4, 1)->addWidget(tl_dz_);

    tl_r_->setRange(0, 100000);
    tl_r_->setSingleStep(1000);
    tl_r_->setValue(10000);

    tl_dr_->setRange(1, 10000);
    tl_dr_->setSingleStep(10);
    tl_dr_->setValue(10);

    tl_z_->setRange(-10000, 0);
    tl_z_->setSingleStep(10);
    tl_z_->setValue(-400);
    
    tl_dz_->setRange(1, 1000);
    tl_dz_->setSingleStep(1);
    tl_dz_->setValue(1);

    z_ = tl_z_->value();
    dz_ = tl_dz_->value();
    r_ = tl_r_->value();
    dr_ = tl_dr_->value();

    
    tl_request_->clicked().connect([this](const WMouseEvent& ev)
				   {
				       iBellhopRequest req;

				       static std::atomic<int> id(2<<16);
				       req.set_request_number(id);				       
				       
				       ++id;
				       
				       auto& env = *req.mutable_env();
				       auto& ai = *env.mutable_adaptive_info();
				       ai.set_contact(tl_tx_->currentText().narrow());
				       transmitter_tcp_port_ = std::stoi(tl_tx_->currentText().narrow());
				       // iBellhop defines ownship as the moving AUV, but we'll use the same as tx,
				       // since we're defining a grid of receivers				       
				       ai.set_ownship(tl_tx_->currentText().narrow());
				       ai.set_auto_receiver_ranges(false);
				       ai.set_auto_source_depth(true);
				       
				       // TODO - fix me
				       env.set_freq(4000);
				       auto& output = *env.mutable_output();
				       output.set_type(bellhop::protobuf::Environment::Output::INCOHERENT_PRESSURE);
				       auto& rx = *env.mutable_receivers();
				       auto& tx = *env.mutable_sources();

				       z_ = tl_z_->value();
				       dz_ = tl_dz_->value();
				       r_ = tl_r_->value();
				       dr_ = tl_dr_->value();
				       
				       rx.set_number_in_depth(std::abs(z_/dz_));
				       rx.set_number_in_range(r_/dr_);
				       rx.mutable_first()->set_depth(0);
				       rx.mutable_first()->set_range(0);
				       rx.mutable_last()->set_depth(std::abs(z_));
				       rx.mutable_last()->set_range(r_);

				       tx.set_number_in_depth(1);


				       auto beams = *env.mutable_beams();
				       beams.set_approximation_type(bellhop::protobuf::Environment::Beams::GAUSSIAN);
				       beams.set_theta_min(-60);
				       beams.set_theta_max(60);
				       beams.set_number(1000);				       
				       
				       this->post_to_comms([=]() { this->goby_thread()->interprocess().publish<groups::bellhop_request>(req); });
				       tl_request_->disable();
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
	    spect_box_->setTitle("Audio Spectrogram (Packet #" + std::to_string(event.packet_id())+ ")");
	    spect_image_resource_.reset(new WFileResource(spect_out_path.str().c_str()));
	    Wt::WLink link(spect_image_resource_.get());
	    spect_image_->setImageLink(link);
	}
	{
	    timeseries_box_->setTitle("Audio Timeseries (Packet #" + std::to_string(event.packet_id()) + ")");
	    timeseries_image_resource_.reset(new WFileResource(timeseries_out_path.str().c_str()));
	    Wt::WLink link(timeseries_image_resource_.get());
	    timeseries_image_->setImageLink(link);
	}
    }
    else if(event.event() == LoggerEvent::PACKET_START)
    {
	if(event.tx_modem_id() < manager_cfg_.sim_env_pair_size())
	{
	    glog.is_debug1() && glog << "Packet start detected" << std::endl;

	    auto tcp_port = manager_cfg_.sim_env_pair(event.tx_modem_id()).modem_tcp_port();
	    nav_at_previous_tx_start_[tcp_port] = nav_at_last_tx_start_[tcp_port];
	    nav_at_last_tx_start_[tcp_port] = last_nav_;

	    glog.is_debug1() && glog << "Navs updated" << std::endl;

	    // paint any updates from previous round
	    tl_plot_->update(Wt::PaintUpdate);
	}
    }
}

void LiaisonNetsim::handle_bellhop_resp(const iBellhopResponse& resp)
{
    tl_image_path_ = resp.output_file() + ".png";
    tl_plot_resource_.reset(new WFileResource(tl_image_path_.c_str()));
    tl_bellhop_resp_ = resp;
//    Wt::WLink link(tl_plot_resource_.get());
//    tl_plot_->setImageLink(link);
    tl_plot_->update();
    tl_request_->enable();

    nav_at_last_tx_start_.clear();
    nav_at_previous_tx_start_.clear();
    receive_stats_.clear();    	      
}

void LiaisonNetsim::handle_manager_cfg(const NetSimManagerConfig& cfg)
{
    manager_cfg_ = cfg;
    
    if(tl_tx_->count() != 0)
	return;

    for(const auto& sim_env_pair : cfg.sim_env_pair())
    {
	tl_tx_->addItem(std::to_string(sim_env_pair.modem_tcp_port()));
//	tl_rx_->addItem(std::to_string(sim_env_pair.modem_tcp_port()));
    }

//  if(tl_rx_->count() > 1)
//	tl_rx_->setCurrentIndex(1);

    
}


void TLPaintedWidget::paintEvent(Wt::WPaintDevice *paintDevice)
{
    Wt::WPainter painter(paintDevice);

    if(!netsim_->tl_image_path_.empty())
    {   	
	Wt::WPainter::Image image(netsim_->tl_plot_resource_->url(), netsim_->tl_image_path_);
	int tick_length = 10;
	resize(image.width()+text_length_, image.height()+text_length_);	

	painter.drawImage(text_length_, 0, image);       

	int tick_spacing = 100; //pixels

	Wt::WPen white_pen = Wt::WPen(Wt::WColor(255,255,255));
	int pwidth= 2;
	white_pen.setWidth(pwidth);
	painter.setPen(white_pen);
	
	for(int y = pwidth/2; y < image.height(); y += tick_spacing)
	    painter.drawLine(text_length_, y, text_length_+tick_length, y);

	for(int x = pwidth/2+text_length_; x < image.width(); x += tick_spacing)
	    painter.drawLine(x, image.height()-tick_length, x, image.height());


	Wt::WPen black_pen = Wt::WPen(Wt::WColor(0,0,0));
	painter.setPen(black_pen);

	for(int y = pwidth/2; y < image.height(); y += tick_spacing)
	    painter.drawText(0, y, text_length_, text_length_, Wt::AlignRight | Wt::AlignTop, std::to_string(-(y-pwidth/2)*netsim_->dz_) + " m");
	    
	for(int x = pwidth/2+text_length_; x < image.width(); x += tick_spacing)
	    painter.drawText(x, image.height(), text_length_, text_length_, Wt::AlignLeft | Wt::AlignTop, std::to_string((x-(pwidth/2+text_length_))*netsim_->dr_)+ " m");
	
	netsim_->tl_image_path_.clear();
//	refresh_key_.clear();
    }


    
    auto& navs = netsim_->nav_at_previous_tx_start_[netsim_->transmitter_tcp_port_];
    glog.is_debug1() && glog << "Paint event: navs.size() = " << navs.size() << std::endl;
    if(!navs.empty() && navs.count(netsim_->transmitter_tcp_port_))
    {
	auto& rx_stats = netsim_->receive_stats_[netsim_->transmitter_tcp_port_];
	
	using boost::units::degree::degrees;
	using boost::units::si::meters;

	
	const auto& tx_nav = navs.at(netsim_->transmitter_tcp_port_);
	
	glog.is_debug1() && glog << "calculating for tx: " << tx_nav.ShortDebugString() << std::endl;

	goby::util::UTMGeodesy geo({ tx_nav.lat()*degrees, tx_nav.lon()*degrees });	
	
	for(const auto& nav_pair : navs)
    	{
	    auto rx_modem_tcp_port = nav_pair.first;
    	    if(rx_modem_tcp_port == netsim_->transmitter_tcp_port_)
    		continue;
	    
    	    painter.setPen(netsim_->rx_pens_[rx_modem_tcp_port % netsim_->rx_pens_.size()]);

    	    Wt::WBrush brush;
	    if(rx_stats.count(rx_modem_tcp_port))
	    {		
		const auto& rx_stat = rx_stats.at(rx_modem_tcp_port);

		if(rx_stat.has_mm_stats())
		{
		    const auto& mm_stat = rx_stat.mm_stats();		    
		    auto mse = mm_stat.mse_equalizer();
		    const int min_mse = -15;
		    const int max_mse = 0;		   
		    auto scale = (255*(mse-min_mse))/(max_mse-min_mse);
		    
		    if(scale > 255)
			scale = 255;
		    else if(scale < 0)
			scale = 0;					 

		    if(mm_stat.number_bad_frames() < mm_stat.number_frames())
			painter.setBrush(Wt::WBrush(Wt::WColor(0,255-scale,scale)));
		    else
			painter.setBrush(Wt::WBrush(Wt::WColor(255,0,0)));
		}
		else if(!rx_stat.packet_success())
		{
		    painter.setBrush(Wt::WBrush(Wt::WColor(255,0,0)));
		}
		else
		{
		    painter.setBrush(Wt::WBrush(Wt::WColor(0,255,0)));
		}		    
	    }
	    else
	    {
		painter.setBrush(Wt::WBrush(Wt::WColor(0,0,0)));
	    }
	    

    	    auto depth = nav_pair.second.depth();

	    auto xy = geo.convert({ nav_pair.second.lat()*degrees, nav_pair.second.lon()*degrees });

	    auto x = xy.x/meters;
	    auto y = xy.y/meters;
	    
	    auto range = std::sqrt(x*x+y*y);

	    glog.is_debug1() && glog << "painting for rx: " << rx_modem_tcp_port << " at range: " << range << " and depth " << depth << std::endl;

	    if(rx_modem_tcp_port % 2 == 0)
		painter.drawEllipse(range/netsim_->dr_+text_length_, depth/netsim_->dz_, diam_, diam_);
	    else
		painter.drawRect(range/netsim_->dr_+text_length_, depth/netsim_->dz_, diam_, diam_);

	    // // key
	    // if(!refresh_key_.count(rx_modem_tcp_port))
	    // {

	    // 	int key_x = this->width().toPixels()-100;
	    // 	auto spacing = 20;
	    // 	auto key_y = (rx_modem_tcp_port-62000)*spacing;

	    // 	glog.is_debug1() && glog << "key_x: " << key_x << ", key_y: "<< key_y << std::endl;
		
	    // 	painter.setBrush(Wt::WBrush(Wt::WColor(255,255,255)));
	    // 	if(rx_modem_tcp_port % 2 == 0)		   
	    // 	    painter.drawEllipse(key_x, key_y, diam_, diam_);
	    // 	else
	    // 	    painter.drawRect(key_x, key_y, diam_, diam_);
	    // 	painter.drawText(key_x + diam_ + 2, key_y, 100, spacing, Wt::AlignLeft | Wt::AlignTop, std::to_string(rx_modem_tcp_port));
	       
	    // 	refresh_key_.insert(rx_modem_tcp_port);		   
	    // }
    	}

	navs.clear();
	rx_stats.clear();
    }
}
