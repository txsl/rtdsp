ord = 4;
rp = 0.5;           % Passband ripple
rs = 25;          % Stopband ripple
fs = 8000;        % Sampling frequency
pb = [280 470];

[a,b] = ellip(ord/2, rp, rs, pb/(fs/2));

freqz(a, b,10)

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
