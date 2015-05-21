#!/usr/bin/env python

import sys
import collections

def load_runs(filename):
	with open(filename, 'r') as infile:
		line = ''
		while True:
			while not line.startswith('Input edge file'): 
				line = next(infile)  # raises StopIteration, ending the generator
				continue  # find next entry

			entry = collections.defaultdict(int)
			key, value = map(str.strip, line.split(':', 1));
			entry[key] = value
			for line in infile:
				line = line.strip()
				if not line: break
				if ':' in line:
					key, value = map(str.strip, line.split(':', 1))
					value = ''.join([c for c in value if c in '1234567890.'])
					entry[key] = value

			yield entry

if len(sys.argv) == 1:
	print "You can also give filename as a command line argument"
	filename = raw_input("Enter Filename: ")
else:
	filename = sys.argv[1]

fields = set();
for run in load_runs(filename):
	fields = fields.union(set(run.keys()))

header = list(fields);
print ','.join(str(column) for column in header)
for run in load_runs(filename):
	print ','.join(str(run[column]) for column in header)
	