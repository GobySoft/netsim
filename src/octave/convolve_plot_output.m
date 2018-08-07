pkg load signal
close all;
files=glob('/tmp/convolvetest_elem_0_frame_*365');
%files{1} = '/tmp/noise.bin';


fs = 96000;
for file_i = 1:length(files)
  file = files{file_i};
  fid = fopen(file);
  packet_time = fread(fid,1,'double');
  data = fread(fid,Inf,'float');  
  time = (1.0:size(data))/fs;
  fclose(fid);
   
  figure(file_i)
%  subplot(length(files), 1, file_i);
  plot(time, data);
end
