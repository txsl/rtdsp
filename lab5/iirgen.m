ord = 4;
rp = 0.5;           % Passband ripple
rs = 25;          % Stopband ripple
fs = 8000;        % Sampling frequency
pb = [280 470];

%divide by two as the ellip function's first parameter is doubled
[b,a] = ellip(ord/2, rp, rs, pb/(fs/2));
figure(1)
%plot the fequency spectrum
freqz(b,a,2000)
figure(2)
%plot z plane
zplane(b,a)


out = 'double a[] = {';

for i=1:length(a)
    if i==1
        out = [out, sprintf('%1.16e', a(i))];
    else
        out = [out, ', ', sprintf('%1.16e', a(i))];
    end
    
end
out = [out, '};'];

out = [out, '\n', 'double b[] = {'];

for i=1:length(b)
    if i==1
        out = [out, sprintf('%1.16e', b(i))];
    else
        out = [out, ', ', sprintf('%1.16e', b(i))];
    end
    
end
out = [out, '};'];

out = [out, '\n', '#define ORDER ', sprintf('%d', ord), '\n'];

%Write it to a file
f = fopen('RTDSP/coeffs.txt', 'w+');
fprintf(f, out);
fclose(f);
