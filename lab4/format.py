handle = open('RTDSP/coeffs.txt', 'r+') # open the file and read its contents
coeffs = handle.readline()
handle.close()

split = coeffs.split('\t') # split the file by the tab delimiter which matlab exported it

out = 'double b[] = {' # the start of the string 

# we iterate through the coefficients and append them to the string, with a comma
for c in split: 
	if c == '\n':  # if there is a newline character, we ignore that iteration
		continue
	out = out + c + ','

out = out[0:-1] # take the last comma from the string 
out = out + '}' # add the closing brace

# overwrite the original file with this output
new = open('RTDSP/coeffs.txt', 'w+')
new.write(out)