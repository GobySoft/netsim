fs = 96000;
fc = 4000;
bw = 1000;
T = 1.0;
ts = 1.0;
dt = 1.0/fs;
t = dt*(0:1:ts/dt);
%sig = [sin(2*pi*fc*(0:1:floor(ts*fs)) * dt), ...
%    zeros(1,floor((T-ts)*fs))];
sig = [chirp(t,fc-bw/2,ts,fc+bw/2), ...
    zeros(1,floor((T-ts)*fs))];
   
t = dt*(0:1:length(sig)-1);
plot(t,sig);
fid = fopen(['LFM_' num2str(fc) '_' num2str(bw) '_' num2str(ts) 's.dat'],'w');
for i=1:length(sig);
    fwrite(fid,sig(i),'float32');
end
fclose(fid);

