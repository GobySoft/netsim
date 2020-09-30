function [ire] = parse_ire(filename);

ire = [];
fd = fopen(filename, 'r');
count = 1;
count2 = 1;
linecnt = 0;

while 1
    line = fgetl(fd);
    linecnt = linecnt+1;
    if ~isstr(line),break,end;
    line(1:findstr(line,'$')-1) = [];
    if (strncmp(line,'$CAIRE,1', 8))
        irestr = line(10:findstr(line,'*')-1);
        npts = length(irestr)/4;
        if ((npts - fix(npts)) == 0)
            irestr = reshape(irestr,4,npts);
            try
                ire(1:npts,count2) = hex2dec(irestr.');
            catch
                ire(1:npts,count2) = 0;
            end
        end
        % now get the 2nd ire buffer
        line = fgetl(fd);
        linecnt = linecnt + 1;
        if (strncmp(line,'$CAIRE,2', 8))
            irestr = line(10:findstr(line,'*')-1);
            npts = length(irestr)/4;
            if ((npts - fix(npts)) == 0)
                irestr = reshape(irestr,4,npts);
                ire(npts+1:npts*2, count2) =  hex2dec(irestr.');
            end
        else
            count2 = count2+1;
        end
        count2 = count2+1;
        count = count+1;
    end
end
%plot(ire)
fclose(fd);

