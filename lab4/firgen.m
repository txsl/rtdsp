rp = 0.4;           % Passband ripple
rs = 48;          % Stopband ripple
fs = 8000;        % Sampling frequency
f = [240 440 2000 2300];    % Cutoff frequencies
a = [0 1 0];        % Desired amplitudes
% Compute deviations
dev = [10^(-rs/20) (10^(rp/20)-1)/(10^(rp/20)+1)  10^(-rs/20)];

[n,fo,ao,w] = firpmord(f,a,dev,fs);

b = firpm(n+10,fo,ao,w);

save('RTDSP/coeffs.txt', 'b','-ascii', '-double', '-tabs')

out = 'double b[] = {';

for i=1:length(b)
    if i==1
        out = [out, sprintf('%1.16e', b(i))];
    else
        out = [out, ', ', sprintf('%1.16e', b(i))];
    end
    
end
out = [out, '};']

f = fopen('RTDSP/coeffs.txt', 'w+');
fprintf(f, out);
fclose(f);

% freqz(b,1,1024,fs);
% title('Lowpass Filter Designed to Specifications');
