import logging
import sys
logging.basicConfig(stream=sys.stderr)
sys.path.insert(0, 'home/matt/water')
from water import app as application
application.secret_key = "change_me_key"
