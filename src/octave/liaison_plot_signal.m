pkg prefix /home/toby/octave;
pkg local_list /home/toby/.octave_packages
pkg global_list /home/toby/.octave_packages
pkg load signal
close all;
dir=argv(){1};
run_start=argv(){2};
packet_id=str2num(argv(){3});
%dir='/home/toby/Desktop/netsim_logs/audio'
%run_start='20180814T183836'
%packet_id=5
    
in_files=glob([dir '/netsim_' run_start '_in_' sprintf('%03d', packet_id) '*.bin']);
out_files=glob([dir '/netsim_' run_start '_out_' sprintf('%03d', packet_id) '*.bin']);
num_files=length(in_files)+length(out_files);

fs = 96000;

out_packet_time=NaN(size(out_files));
max_time = 15;
for fi = 1:num_files
  if fi == 1    
    file = strsplit(in_files{1}, '/'){end};
    fid = fopen(in_files{1});
    in_packet_time = fread(fid,1,'double');
    dt(fi) = 0;
  else
    file = strsplit(out_files{fi-1}, '/'){end};
    fid = fopen(out_files{fi-1});
    out_packet_time(fi-1) = fread(fid,1,'double');
    dt(fi) = out_packet_time(fi-1) - in_packet_time;
  end
  
  data{fi} = fread(fid,Inf,'float');  
  time{fi} = (1.0:size(data{fi}))/fs + dt(fi);
  %max_time = max(max_time, max(time{fi}));
  fclose(fid);
end
  
for fi = 1:num_files  
if fi == 1    
    file = strsplit(in_files{1}, '/'){end};
    fid = fopen(in_files{1});
    in_packet_time = fread(fid,1,'double');
    dt(fi) = 0;
  else
    file = strsplit(out_files{fi-1}, '/'){end};
    fid = fopen(out_files{fi-1});
    out_packet_time(fi-1) = fread(fid,1,'double');
    dt(fi) = out_packet_time(fi-1) - in_packet_time;
  end

  figure(1, "visible", "off")
  subplot(num_files, 1, fi);
  
  downsample_factor=20;
  % downsample to reduce visual data to render
  plot(downsample(time{fi},downsample_factor), downsample(data{fi}, downsample_factor));
  xlim([0 max_time]);
  
  ylabel('amplitude');

  if fi == num_files  
      xlabel('time (s) since detection start')
  end

  [re_s, re_e, re_te, re_m, re_t, re_nm, re_sp]= regexp(file, "_modem([0-9]+)\.bin");
  port=str2num(re_t{1}{1})+62000;
      
  if fi == 1
      title_str=["Transmitter " num2str(port)];
  else
      title_str=["Receiver " num2str(port)];
  end

  title(title_str);

  figure(2, "visible", "off")
  subplot(num_files, 1, fi);
  [S, f, t] = specgram(data{fi}, 2048, 96000);
  S = 20*log10(abs(S));
  imagesc(t + dt(fi), f, S);
  xlim([0 max_time]);
  set(gca,'YDir','normal')
  colorbar
  title(title_str);
  if fi == num_files  
      xlabel('time (s) since detection start')
  end
end

xsize = 1000;
ysize = 400;

figure(1, "visible", "off")
print([dir "/netsim_" run_start "_" sprintf('%03d', packet_id) "_timeseries.png"],'-dpng',['-S' num2str(xsize) ',' num2str(ysize)]);

figure(2, "visible", "off")
print([dir "/netsim_" run_start "_" sprintf('%03d', packet_id) "_spectrogram.png"],'-dpng',['-S' num2str(xsize) ',' num2str(ysize)]);
%print([dir "/netsim_" run_start "_" sprintf('%03d', packet_id) "_spectrogram.png"],'-dpng');

