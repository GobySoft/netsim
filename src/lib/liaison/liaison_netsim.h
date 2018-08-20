
#ifndef LIAISONNETSIM20180820H
#define LIAISONNETSIM20180820H

#include <Wt/WText>
#include <Wt/WCssDecorationStyle>
#include <Wt/WBorder>
#include <Wt/WColor>
#include <Wt/WVBoxLayout>
#include <Wt/WImage>
#include <Wt/WPushButton>
#include <Wt/WGroupBox>
#include <Wt/WDoubleSpinBox>

#include "goby/middleware/liaison/liaison_container.h"
#include "goby/middleware/multi-thread-application.h"

#include "messages/liaison.pb.h"
#include "messages/groups.h"
#include "messages/logger.pb.h"
#include "messages/config_request.pb.h"
#include "messages/manager_config.pb.h"

class NetsimCommsThread;
    
class LiaisonNetsim : public goby::common::LiaisonContainerWithComms<LiaisonNetsim,
    NetsimCommsThread>
{
public:
    LiaisonNetsim(const goby::common::protobuf::LiaisonConfig& cfg);

    void handle_new_log(const LoggerEvent& event);
    void handle_manager_cfg(const NetSimManagerConfig& cfg);
   
    
private:
    
    
    const protobuf::LiaisonNetsimConfig& netsim_cfg_;

    Wt::WGroupBox* timeseries_box_;
    Wt::WImage* timeseries_image_;
    std::unique_ptr<Wt::WResource> timeseries_image_resource_;
    Wt::WGroupBox* spect_box_;
    Wt::WImage* spect_image_;
    std::unique_ptr<Wt::WResource> spect_image_resource_;

    Wt::WGroupBox* tl_box_;
    Wt::WTable* tl_table_;
    
    Wt::WText* tl_tx_txt_;
    Wt::WComboBox* tl_tx_;

    Wt::WText* tl_rx_txt_;
    Wt::WComboBox* tl_rx_;    

    Wt::WText* tl_r_txt_;
    Wt::WSpinBox* tl_r_;
    
    Wt::WText* tl_dr_txt_;
    Wt::WSpinBox* tl_dr_;

    Wt::WText* tl_z_txt_;
    Wt::WSpinBox* tl_z_;

    Wt::WText* tl_dz_txt_;
    Wt::WSpinBox* tl_dz_;
    
    Wt::WPushButton* tl_request_;
    
};
    
     
class NetsimCommsThread : public goby::common::LiaisonCommsThread<LiaisonNetsim>
{
public:
NetsimCommsThread(LiaisonNetsim* wt_app, const goby::common::protobuf::LiaisonConfig& config, int index) :
    goby::common::LiaisonCommsThread<LiaisonNetsim>(wt_app, config, index),
        wt_app_(wt_app)
        {
            interprocess().subscribe<groups::post_process_event,
                LoggerEvent>(
                    [this](const LoggerEvent& event)
                    {
                        wt_app_->post_to_wt(
                            [=]() { wt_app_->handle_new_log(event); });
                    });
	    
            interprocess().subscribe<groups::configuration,
                NetSimManagerConfig>(
                    [this](const NetSimManagerConfig& manager_cfg)
                    {
                        wt_app_->post_to_wt(
                            [=]() { wt_app_->handle_manager_cfg(manager_cfg); });
                    });

	    
	    ConfigRequest req;
	    req.set_subsystem(ConfigRequest::MANAGER);
	    interprocess().publish<groups::config_request>(req);
	    
        }
    ~NetsimCommsThread()
    {
    }
            
private:
    friend class LiaisonNetsim;
    LiaisonNetsim* wt_app_;
            
};

#endif
