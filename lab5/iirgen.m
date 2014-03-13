ord = 4;
rp = 0.5;           % Passband ripple
rs = 25;          % Stopband ripple
fs = 8000;        % Sampling frequency
pb = [280 470];

[z,p,k] = ellip(ord, rp, rs, pb/(fs/2));
[a,b] = ellip(ord, rp, rs, pb/(fs/2))
[sos,g] = zp2sos(z,p,k);
Hd = dfilt.df2tsos(sos,g);
h = fvtool(Hd);
