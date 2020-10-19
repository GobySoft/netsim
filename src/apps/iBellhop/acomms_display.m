close all;
clear iMatlab;

iMatlab('INIT','CONFIG_FILE','acomms_display.moos','MOOSNAME','acomms_depth_disp');
iMatlab('MOOS_PAUSE',0.1);

vehicle1='micromodem_gateway';
vehicle2='unicorn';

x1 = 0;
y1 = 0;
depth1 = 0;
time1 = 0;

x2 = 0;
y2 = 0;
depth2 = 0;
time2 = 0;

if(~exist('r'))
    r = [];
    z = [];
end


new_point =0;

while (1)
    
    figure(1);
    directory = '/media/LAMSS001B/GLINT10/2010Aug13/topside';
    files = dir([ directory '/*micromodem*.shd' ]);

    max_num_subplots = 3;
    end_it = max(length(files)-max_num_subplots,1);
    num_subplots = length(files) - end_it;

    subplot_it = 1;
    tlt_mean = NaN;
    for it = length(files)-1:-1:end_it
        this_file = files(it).name;
        full_path = [directory '/' this_file];
        
        % from m. porter's plotshd
        [ PlotTitle, PlotType, freq, atten, Pos, pressure ] = read_shd(full_path);
        pressure = squeeze(pressure(1, 1, :, :));
        zt = Pos.r.depth;
        rt = Pos.r.range;

        tlt = abs(pressure);

        tlt( isnan( tlt ) ) = 1e-6;   % remove NaNs
        tlt( isinf( tlt ) ) = 1e-6;   % remove infinities

        icount = find( tlt > 1e-7 );         % for stats, only these values count
        tlt( tlt < 1e-7 ) = 1e-7;            % remove zeros
        tlt = -20.0 * log10( tlt );          % so there's no error when we take the log

        % compute some statistics to automatically set the color bar

        tlmed = median( tlt( icount ) );    % median value
        tlstd = std( tlt( icount ) );       % standard deviation
        tlmax = tlmed + 0.75 * tlstd;       % max for colorbar
        tlmax = 10 * round( tlmax / 10 );   % make sure the limits are round numbers
        tlmin = tlmax - 50;                 % min for colorbar

        if(subplot_it == 1)
            tlt_mean = tlt;
        else
            tlt_mean = (tlt + tlt_mean*(subplot_it-1))/subplot_it;
        end
        
        % end m. porter            
        
        subplot(num_subplots+1, 5, [5*subplot_it+2 5*(subplot_it+1)]);
        imagesc(rt, zt, tlt);
        colormap(gray);
        caxisrev( [ tlmin, tlmax ] )
        colorbar;

        title([ PlotTitle ' ' files(it).date]);
            
        subplot(num_subplots+1, 5, 5*(subplot_it)+1);
        [ pltitle, freq, SSP, Bdry, fid ] = read_env(full_path(1:end-4), 'KRAKEN')
        plot(SSP.c, SSP.z); axis ij;
        grid on;
        
        subplot_it = subplot_it + 1;
        
    end

    
    fclose all;

    mail=iMatlab('MOOS_MAIL_RX');
    messages=length(mail);

    % Process messages
    for m=1:messages

        key=mail(m).KEY;
        s=mail(m).STR;

        switch key
            case 'NODE_REPORT'
                lhs = s(findstr('NAME=',s) + length('NAME='):end);
                name = lhs(1:findstr(',',lhs)-1);

                lhs = s(findstr('X=',s) + length('X='):end);
                x = str2num(lhs(1:findstr(',',lhs)-1));

                lhs = s(findstr('Y=',s) + length('Y='):end);
                y = str2num(lhs(1:findstr(',',lhs)-1));

                lhs = s(findstr('UTC_TIME=',s) + length('UTC_TIME='):end);
                t = str2num(lhs(1:findstr(',',lhs)-1));
                
                lhs = s(findstr('DEPTH=',s) + length('DEPTH='):end);
                if(findstr(',', lhs))
                   depth = str2num(lhs(1:findstr(',',lhs)-1));
                else
                    depth = str2num(lhs);
                end
                    
                % make sure this is a full status report
                if strcmpi(name,vehicle1) && isempty(findstr('MODE', s)) && t > time1
                    time1 = t;
                    x1 = x;
                    y1 = y;
                    depth1 = depth;
                elseif strcmpi(name,vehicle2)  && isempty(findstr('MODE', s)) && t > time2
                    time2 = t;
                    x2 = x;
                    y2 = y;
                    depth2 = depth;
                    new_point = 1;
                    s
                end
                    
        end
    end

    figure(1);
    subplot(num_subplots+1, 5, [2 5], 'replace');
    imagesc(rt, zt, tlt);
    colormap(gray); hold on;
    caxisrev( [ tlmin, tlmax ] )
    colorbar;    
    if new_point==1
        r(end+1) = sqrt((x1-x2).^2 + (y1-y2).^2)
        z(end+1) = depth2
        new_point = 0;
    end
    plot(r,z,'mo');
    pause(10);

end
