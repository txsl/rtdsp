subplot(2, 1, 1), plot(freq_x, gain_single_pole - clean_gain), grid;
xlabel('Frequency (Hz)')
ylabel('Gain (dB)')
subplot(2, 1, 2), plot(freq_x, phase_single_pole - phase_clean), grid;
xlabel('Frequency (Hz)')
ylabel('Phase Shift (Hz)')

ax = findall(gcf, 'Type', 'axes');
set(ax, 'XScale', 'log');