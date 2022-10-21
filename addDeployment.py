from datetime import datetime
from geoalchemy2 import Geometry
import water

# IMEI = 300534061674790
IMEI = 123456789013245
Location = 'SRID=4326;POINT(53.891425 -122.814566)'
Height = 300

deployments = water.models.Deployment.query.order_by(water.models.Deployment.id.desc()).first()

if deployments == None:
    site = water.models.Deployment(0, IMEI, Location, Height, datetime.now())
else:
    site = water.models.Deployment(deployments.id + 1, IMEI, Location, Height, datetime.now())

water.db.session.add(site)
water.db.session.commit()