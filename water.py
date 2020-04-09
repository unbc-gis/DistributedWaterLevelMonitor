from flask import Flask, Response, request
import sys
from flask_sqlalchemy import SQLAlchemy
from geoalchemy2 import Geometry
from flask_migrate import Migrate
from datetime import datetime

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'postgresql://postgres:PASSWORD@localhost/water_level'
db = SQLAlchemy(app)

migrate = Migrate(app, db)
SQLALCHEMY_TRACK_MODIFICATIONS = False

@app.route('/')
def landing():
    return app.send_static_file('map.html')


@app.route("/rockblock", methods=["POST"])
def rock_block():
    imei = request.args.get('imei')
    momsn = request.args.get('momsn')
    transmit_time = request.args.get('transmit_time')
    iridium_latitude = request.args.get("iridium_latitude")
    iridium_longitude = request.args.get("iridium_longitude")
    iridum_cep = request.args.get("iridium_cep")
    payload = request.args.get('data')

    print(request, file=sys.stdout)

    deployment = 1
    time = datetime.now()
    level = 1
    water_temp = 2
    air_temp = 3
    humidity = 4
    pressure = 5

    entry = Measurement(1, datetime.now(), 1, 2, 3, 4, 5)
    db.session.add(entry)
    db.session.commit()
    return Response(status=200, mimetype='application/json')


if __name__ == '__main__':
    app.run()


class Deployment(db.Model):
    __tablename__ = 'deployment'

    id = db.Column(db.Integer, primary_key=True)
    imei = db.Column(db.Text)
    location = db.Column(Geometry('POINT'))
    height = db.Column(db.Integer)
    deployed = db.Column(db.TIMESTAMP)
    retrieved = db.Column(db.TIMESTAMP)

    def __init__(self, id, imei, location, height, deployed):
        self.id = id
        self.imei = imei
        self.location = location
        self.height = height
        self.deployed = deployed

    def __repr__(self):
        return '<id {}>'.format(self.id)

    def __serialize__(self):
        return {
            'id': self.id,
            'IMEI': self.imei,
            'location': self.location,
            'height': self.height,
            'deployed': self.deployed,
            'retrieved': self.retrieved

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

    def __init__(self, deployment, time, level, water_temp, air_temp, humidity, pressure):
        self.deployment = deployment
        self.time = time
        self.level = level
        self.water_temp = water_temp
        self.air_temp = air_temp
        self.humidity = humidity
        self.pressure = pressure

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
