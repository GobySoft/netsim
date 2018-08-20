pkg load signal
close all;
dir='/home/toby/Desktop/netsim_logs/audio'
run_start='20180814T183836'
packet_id=5
packet_id_end=5

in_files=glob([dir '/netsim_' run_start '_in_' sprintf('%03d', packet_id) '*.bin']);
out_files=glob([dir '/netsim_' run_start '_out_' sprintf('%03d', packet_id) '*.bin']);
num_files=length(in_files)+length(out_files);

fs = 96000;

out_packet_time=NaN(size(out_files));
max_time = 8;
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
  
  data{fi} = fread(fid,Inf,'double');  
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
  
  downsample_factor=10;
  % downsample to reduce visual data to render
  plot(downsample(time{fi},downsample_factor), downsample(data{fi}, downsample_factor));
  xlim([0 max_time]);
  
  ylabel('amplitude');
  xlabel(['time (s) since ' num2str(in_packet_time)]);
  title(file, 'Interpreter', 'None');
  
  figure(2, "visible", "off")
  subplot(num_files, 1, fi);
  [S, f, t] = specgram(data{fi}, 512, 96000);
  S = 20*log10(abs(S));
  imagesc(t + dt(fi), f, S);
  xlim([0 max_time]);
  set(gca,'YDir','normal')
  colorbar
  title(file, 'Interpreter', 'None')
end

xsize = 640;
ysize = 479;

figure(1, "visible", "off")
print([in_files{1} '.timeseries.png'],'-dpng');

figure(2, "visible", "off")
%  print([in_files{1} '.png'],'-dpng',['-S' num2str(xsize) ',' num2str(ysize)]);
print([in_files{1} '.spectrogram.png'],'-dpng');