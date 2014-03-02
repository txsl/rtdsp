f0 = 100;
fs = 8000;
n = 0:120;
len = length(n);
x = sin(2*pi*n*f0/fs);


fspread = (0:len-1)*(fs/len);
f = fft(x, len);
xabs = abs(x);
fabs = fft(xabs, len);


subplot(2, 2, 1)
stem(n,x);
tit = sprintf('Sinusoidal signal of %dHz sampled at %dHz', f0, fs);
title(tit)
xlabel('n')

subplot(2,2,3)
stem(fspread, abs(f));
tit = sprintf('Rectified sinusoidal signal of %dHz sampled at %dHz', f0, fs);
title(tit)
xlabel('Frequency/Hz')

subplot(2,2,2)
stem(n,xabs);
tit = sprintf('Rectified sinusoidal signal of %dHz sampled at %dHz', f0, fs);
title(tit)
xlabel('n')

subplot(2,2,4)
stem(fspread, abs(fabs))
tit = sprintf('Fourier Transform of the rectified %dHz sine wave sampled at %dHz', f0, fs);
title(tit)
xlabel('Frequency/Hz')