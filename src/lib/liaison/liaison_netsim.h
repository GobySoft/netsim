
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

class NetsimCommsThread;
    
class LiaisonNetsim : public goby::common::LiaisonContainerWithComms<LiaisonNetsim,
    NetsimCommsThread>
{
public:
    LiaisonNetsim(const goby::common::protobuf::LiaisonConfig& cfg);
        
private:
        
    const protobuf::LiaisonNetsimConfig& netsim_cfg_;
 

};
    
     
class NetsimCommsThread : public goby::common::LiaisonCommsThread<LiaisonNetsim>
{
public:
NetsimCommsThread(LiaisonNetsim* wt_app, const goby::common::protobuf::LiaisonConfig& config, int index) :
    goby::common::LiaisonCommsThread<LiaisonNetsim>(wt_app, config, index),
        wt_app_(wt_app)
        {
            /* interprocess().subscribe<dsl::progressive_imagery::groups::updated_image, */
            /*     dsl::protobuf::UpdatedImageEvent>( */
            /*         [this](const dsl::protobuf::UpdatedImageEvent& updated_image) */
            /*         { */
            /*             wt_app_->post_to_wt( */
            /*                 [=]() { wt_app_->handle_updated_image(updated_image); });    */
            /*         }); */

            /* interprocess().subscribe<dsl::progressive_imagery::groups::received_status, */
            /*     dsl::protobuf::ReceivedStatus>( */
            /*         [this](const dsl::protobuf::ReceivedStatus& status) */
            /*         { */
            /*             wt_app_->post_to_wt( */
            /*                 [=]() { wt_app_->handle_received_status(status); }); */
            /*         }); */
                
        }
    ~NetsimCommsThread()
    {
    }
            
private:
    friend class LiaisonNetsim;
    LiaisonNetsim* wt_app_;
            
};

#endif
