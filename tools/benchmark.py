import sys
import numpy as np
from scipy import stats
import subprocess as sp
import datetime
import socket
import os

exec_name = sys.argv[1]
max_t = int(sys.argv[2])

ntries = 5

tot_timings = []

for t_idx in range(1,max_t + 1):
	cur_timings = []
	for _ in range(ntries):
		# Run the process.
		p = sp.Popen([exec_name,str(t_idx)],stdout=sp.PIPE,stderr=sp.STDOUT)
		# Wait for it to finish and get stdout.
		out = p.communicate()[0]
		# Parse the stderr in order to find the time.
		out = out.split(bytes('\n','ascii'))[1].split()[0][0:-1]
		cur_timings.append(float(out))
	tot_timings.append(cur_timings)

tot_timings = np.array(tot_timings)

retval = np.array([np.mean(tot_timings,axis=1),stats.sem(tot_timings,axis=1)])

fmt='{fname}_%Y%m%d%H%M%S'
filename = datetime.datetime.now().strftime(fmt).format(fname=socket.gethostname() + '_' + os.path.basename(exec_name)) + '.txt'
np.savetxt(filename,retval)
