%figure ("visible", "off")

filename=argv(){1}
%filename='/home/toby/gs/research/ucomms2018/arctic/ICEX16_100m_env.shd'

% From M. Porter's Acoustics Toolbox
[ PlotTitle, ~, freq, ~, Pos, pressure ] = read_shd( filename );
itheta = 1;
isd    = 1;

pressure = squeeze( pressure( itheta, isd, :, : ) );
zt       = Pos.r.depth;
rt       = Pos.r.range;

tlt = abs( pressure );
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
%
tlt_max=tlmax;
tlt_min=tlmin;
tlt_scaled=255*(1-(tlt-tlt_min)./(tlt_max-tlt_min));
tlt_scaled(tlt_scaled<0)=0;
imwrite(uint8(tlt_scaled), 'output2.png');

cbar=repmat(linspace(255, 0, length(zt))', 1, 50);
imwrite(uint8(cbar), 'cbar.png');
