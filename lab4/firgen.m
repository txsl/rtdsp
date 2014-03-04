rp = 0.4;           % Passband ripple
rs = 48;          % Stopband ripple
fs = 8000;        % Sampling frequency
f = [240 440 2000 2300];    % Cutoff frequencies
a = [0 1 0];        % Desired amplitudes
% Compute deviations
dev = [10^(-rs/20) (10^(rp/20)-1)/(10^(rp/20)+1)  10^(-rs/20)]
[n,fo,ao,w] = firpmord(f,a,dev,fs)
b = firpm(n,fo,ao,w);
freqz(b,1,1024,fs);
title('Lowpass Filter Designed to Specifications');

% f = [.03 .055 .25 .30625]; % these correspond to the different frequencies in proportion to the nyquist plot