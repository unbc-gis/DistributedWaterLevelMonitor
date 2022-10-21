from water import db
from geoalchemy2 import Geometry
import geoalchemy2
from shapely_geojson import dumps, Feature
from geoalchemy2.shape import to_shape

class Deployment(db.Model):
    __tablename__ = 'deployment'

    id = db.Column(db.Integer, primary_key=True)
    imei = db.Column(db.Text)
    location = db.Column(Geometry('POINT'))
    height = db.Column(db.Integer)
    deployed = db.Column(db.TIMESTAMP)
    retrieved = db.Column(db.TIMESTAMP)
    url = db.Column(db.Text)

    def __init__(self, id, imei, location, height, deployed):
        self.id = id
        self.imei = imei
        self.location = location
        self.height = height
        self.deployed = deployed
        self.url = 'http://142.207.145.101/results?deployment=' + str(id)

    def __repr__(self):
        return '<id {}>'.format(self.id)

    def __serialize__(self):
        return {
            'id': self.id,
            'IMEI': self.imei,
            'location': dumps(Feature(to_shape(self.location))),
            'height': self.height,
            'deployed': self.deployed,
            'retrieved': self.retrieved,
            'url': self.url
        }


class Measurement(db.Model):
    __tablename__ = 'measurement'

    deployment = db.Column(db.Integer, db.ForeignKey('deployment.id'), primary_key=True)
    time = db.Column(db.TIMESTAMP, primary_key=True)
    level = db.Column(db.Numeric)
    water_temp = db.Column(db.Numeric)
    air_temp = db.Column(db.Numeric)
    humidity = db.Column(db.Numeric)
    pressure = db.Column(db.Numeric)
    imei = db.Column(db.Text)
    lat = db.Column(db.Numeric)
    lon = db.Column(db.Numeric)
    datasource = db.Column(db.Text)

    def __init__(self, deployment, time, level, water_temp, air_temp, humidity, pressure, imei, lat, lon, datasource):
        self.deployment = deployment
        self.time = time
        self.level = level
        self.water_temp = water_temp
        self.air_temp = air_temp
        self.humidity = humidity
        self.pressure = pressure
        self.imei = imei
        self.lat = lat
        self.lon = lon
        self.datasource = datasource

    def __repr__(self):
        return '<id {}>'.format(self.deployment, self.time)

    def __serialize__(self):
        return {
            'deployment': self.deployment,
            'time': self.time,
            'level': self.level,
            'water_temp': self.water_temp,
            'air_temp': self.water_temp,
            'humidity': self.humidity,
            'pressure': self.pressure
        }