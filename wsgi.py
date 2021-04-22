#import logging
#import sys
#logging.basicConfig(stream=sys.stderr)
#sys.path.insert(0, '/water_server')
from water import app 
if __name__ == '__main__':
	app.run()
