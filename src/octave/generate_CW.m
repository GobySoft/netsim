fs = 96000;
fc = 4000;
T = 5.0;
ts = 1.0;
dt = 1.0/fs;
sig = [sin(2*pi*fc*(0:1:floor(ts*fs)) * dt), ...
    zeros(1,floor((T-ts)*fs))];
   
t = dt*(0:1:length(sig)-1);
plot(t,sig);
fid = fopen('CW_4000_1s.dat','w');
for i=1:length(sig);
    fwrite(fid,sig(i),'float32');
end
fclose(fid);

