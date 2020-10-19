close all;
clear iMatlab;

iMatlab('INIT','CONFIG_FILE','acomms_depth_vis.moos','MOOSNAME','acomms_depth_vis');
iMatlab('MOOS_PAUSE',0.1);


while (1)

    mail=iMatlab('MOOS_MAIL_RX');
    messages=length(mail);

    % Process messages
    for m=1:messages

        key=mail(m).KEY;
        s=mail(m).STR;

        switch key
            case 'BHV_ACOMMS_DEPTH_FUNC'
                A = textscan(s, 'domain={%[-0123456789.,]},range={%[-0123456789.,]}');
                range = textscan(A{2}{1}, '%f', 'delimiter', ',');
                domain = textscan(A{1}{1}, '%f', 'delimiter', ',');
                plot(range{1}, domain{1}); axis ij;
        end
    end
    pause(1);
end
