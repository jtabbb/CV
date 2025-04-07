import time
import subprocess
try:
	while True:
		print('Running..?')
		subprocess.call(['sh','/home/tabbet_j-adm/Desktop/drs-5.0.5/DataOutput/Update.sh'])
		time.sleep(900)
except KeyboardInterrupt:
	pass
