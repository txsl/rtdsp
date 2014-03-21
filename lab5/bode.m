R = 1e3;
C = 1e-6;

sys = tf([1], [R*C 1]);

bode(sys);

figure;
freqz([1/17 1/17], [1 -15/17],1024, 8000)
ax = findall(gcf, 'Type', 'axes');
set(ax, 'XScale', 'log');