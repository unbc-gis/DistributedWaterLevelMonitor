from datetime import datetime
from geoalchemy2 import Geometry
import water

deployments = water.models.Deployment.query.order_by(water.models.Deployment.id.desc()).first()
site = water.models.Deployment(deployments.id + 1, 300534060910660, 'SRID=4326;POINT(53.7943 -122.7791)', 300, datetime.now())
water.db.session.add(site)
water.db.session.commit()
