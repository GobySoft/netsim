#include "lamss/lib_henrik_util/CConvolve.h"
#include <chrono>

void write_file(std::string file_name, double timestamp, const std::vector<double>& signal);

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
	std::cerr << "Usage: cconvolve_test path_to_replica" << std::endl;
	exit(1);
    }    
   
    const char* replica_file = argv[1];

    std::vector<double> noise;
    std::vector<std::vector<double>> full_signal;
    const int frame_size = 1024;
    const int sampling_freq = 96000;
    const double source_calibration_db = 180;
    const double receiver_calibration_db = 60;
    const double noise_level = 40;
    const double acomms_freq = 25000;
    const double surface_rms_roughness = 0;
    const double surface_roughness_loss = 0;
    
    
    // generate 10 seconds of noise
    CConvolve temp;
    temp.create_noise(10.0*sampling_freq, noise);
    //write_file("/tmp/noise.bin", 0, noise);
    
    
    CConvolve convolve;   
    double timestamp;

    ImpulseResponse impulse_response;
  //   impulse_response.set_source("src");
  //   impulse_response.set_source("dst");
  //   {
  // 	auto* raytrace = impulse_response.add_raytrace();
  // 	raytrace->add_element()->set_delay(.5);
  // 	raytrace->set_amplitude(1);
  //   }
  //   {
  // 	auto* raytrace = impulse_response.add_raytrace();
  // 	raytrace->add_element()->set_delay(4);
  // 	raytrace->set_amplitude(0.5);
  // }
    std::string imp_rep_str = "source: \"62000\" receiver: \"62001\""
	"raytrace { amplitude: -0.0051012421 doppler: 1.0009956301482308 elevation: 14.6926994 surface_bounces: 1 bottom_bounces: 0 element { delay: 0.1265081630067604 } } "
	"raytrace { amplitude: -3.59404521e-06 doppler: 1.000996831725558 elevation: 14.426631000000002 surface_bounces: 1 bottom_bounces: 0 element { delay: 0.12647255724324247 } } "
	"raytrace { amplitude: 0.00525906309 doppler: 1.0010278467409948 elevation: -3.032197 surface_bounces: 0 bottom_bounces: 0 element { delay: 0.12136817704779784 } } "
	"raytrace { amplitude: 5.20280082e-06 doppler: 1.0010281305607063 elevation: -2.7171731 surface_bounces: 0 bottom_bounces: 0 element { delay: 0.12134185156064167 } } "
	"noise_level: 39.210526315789473 receiver_sound_speed: 1434.17 surface_sound_speed: 1433.64 bottom_sound_speed: 0 request_id: 2669 request_time: 1533310017.000212 ";

    google::protobuf::TextFormat::ParseFromString(imp_rep_str, &impulse_response);
	    

    ArrayGain array_gain;
    
    double ping_time;
    std::vector<float> full_replica;
    {
	std::ifstream ifs(replica_file, std::ios::in | std::ios::binary);
	if(!ifs.is_open())
	{
	    std::cerr << "Failed to open: " << replica_file << std::endl;
	    exit(1);
	}
	auto begin = ifs.tellg();
	ifs.seekg (0, ifs.end);
	auto end = ifs.tellg();
	ifs.seekg(0, ifs.beg);
	auto file_size = end-begin;
	std::cout << "Replica file size: " << file_size << std::endl;
	ifs.read(reinterpret_cast<char*>(&ping_time), sizeof(double));
	full_replica.resize((file_size-sizeof(double))/sizeof(float));
	ifs.read(reinterpret_cast<char*>(&full_replica[0]), full_replica.size()*sizeof(float));
    }

    auto start = std::chrono::system_clock::now();

    convolve.initialize(frame_size,
                        ping_time,
                        sampling_freq,
                        noise_level,
                        acomms_freq,
                        surface_rms_roughness,
                        surface_roughness_loss,
                        source_calibration_db,
                        receiver_calibration_db,
                        impulse_response,
                        array_gain,
                        timestamp,
                        full_signal);
    

    int number_frames = full_replica.size()/frame_size;
    if(full_replica.size() % frame_size != 0)
	number_frames += 1; // last partial frame
    
    for(int i = 0; i <= number_frames; ++i)
    {
	if(i < number_frames)
	{
	    auto begin_it = full_replica.begin() + i*frame_size;
	    auto end_it = full_replica.begin() + (i+1)*frame_size;
	    if(end_it > full_replica.end())
		end_it = full_replica.end();
	    
	    std::vector<double> frame_buffer(begin_it, end_it);
	    frame_buffer.resize(frame_size, 0);
	    convolve.signal_block(frame_buffer, noise, full_signal);
	}
	else // finalize
	{
	    convolve.finalize(noise, full_signal);
	}
        std::stringstream file;

	for(int element = 0, end = full_signal.size(); element < end; ++element)
	{
	    file << "/tmp/convolvetest_elem_" << element << "_frame_" << std::setw(5) << std::setfill('0') << i;
	    write_file(file.str(), ping_time, full_signal.at(element));
	}
    }
    auto end = std::chrono::system_clock::now();
    std::cout << "Took: " << std::chrono::duration<double>(end-start).count() << " sec" << std::endl;
    
}

void write_file(std::string file_name, double timestamp, const std::vector<double>& signal)
{
    std::vector<float> float_signal(signal.begin(), signal.end());
    std::ofstream ofs(file_name.c_str(), std::ios::out | std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(&timestamp), sizeof(double));
    ofs.write(reinterpret_cast<const char*>(&float_signal[0]), float_signal.size()*sizeof(float));
}
