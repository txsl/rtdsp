rp = 0.4;           % Passband ripple
rs = 48;          % Stopband ripple
fs = 8000;        % Sampling frequency
f = [240 440 2000 2300];    % Cutoff frequencies
a = [0 1 0];        % Desired amplitudes
dev = [10^(-rs/20) (10^(rp/20)-1)/(10^(rp/20)+1)  10^(-rs/20)]; % Compute deviations

n_extra = 0; % Additional terms to add before coefficient calculation

[n,fo,ao,w] = firpmord(f,a,dev,fs);
n + n_extra;
b = firpm(n + n_extra,fo,ao,w);

%Generate the file to be #included in the embedded C code
out = 'double b[] = {';

for i=1:length(b)
    if i==1
        out = [out, sprintf('%1.16e', b(i))];
    else
        out = [out, ', ', sprintf('%1.16e', b(i))];
    end
    
end
out = [out, '};'];

%Write it to a file
f = fopen('RTDSP/coeffs.txt', 'w+');
fprintf(f, out);
fclose(f);

freqz(b,1,1024,fs);
title('Lowpass Filter Designed to Specifications');
