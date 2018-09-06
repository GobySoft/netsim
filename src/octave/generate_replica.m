function [source_waveform_replica,t_source,source_waveform]  ...
  = generate_replica(waveform_name,wavfilename,fs_source,samp_freq, ...
                     chirp_width,F0,F1,FC,sourceStrengthdB)

  sourceStrength = 10.0^(sourceStrengthdB/20);

%Get source waveforms if wav file exists, otherwize generate:

  if exist(wavfilename)
    msg =  ['>>>>> Reading replica from ' wavfilename]
    [source_waveform_replica,t_source,source_waveform] = ...
         getwaveform(wavfilename,fs_source,samp_freq,sourceStrengthdB);
    
  else
    msg = ['>>>>>> Generating replica ' waveform_name] 
    t_src = 0:1/fs_source:chirp_width;
%---------- Generate replica waveform using chip for NSRCS # of sources-----

 %    source_waveform(:,1) = [chirp(t_src,F0,t_src(end),F1)];
 %    source_waveform(:,1) = source_waveform(:,NSRCS)*sourceStrength;

   if (waveform_name(end-2:end) == '_CW')
      source_waveform(:,1) = sin(2*pi*FC*t_src)*sourceStrength;
   else
      source_waveform(:,1) = chirp(t_src,F0,t_src(end),F1)*sourceStrength;
   end
 
   t_source(:,1) = ([1:size(source_waveform,1)]-1)/fs_source;
    %replica data:
    
    fs_source_replica = samp_freq;
    t_source_replica = 0:1/fs_source_replica:chirp_width;
   if (waveform_name(end-2:end) == '_CW')
      source_waveform_replica = sin(2*pi*FC*t_source_replica)*sourceStrength;
   else
      source_waveform_replica =  chirp(t_source_replica,F0,t_source_replica(end),F1)*sourceStrength;
   end
  end
end
