handle = open('RTDSP/coeffs.txt', 'r+')
coeffs = handle.readline()
handle.close()

split = coeffs.split('\t')

out = 'double b[] = {'

for c in split:
	if c == '\n':
		continue
	out = out + c + ','

out = out[0:-1]
out = out + '}'

new = open('RTDSP/coeffs.txt', 'w+')
new.write(out)