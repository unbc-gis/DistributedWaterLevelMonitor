from flask import Flask, Response, request, render_template
import sys
from flask_sqlalchemy import SQLAlchemy
from flask_migrate import Migrate
from datetime import datetime, timedelta
from flask import jsonify

app = Flask(__name__)

password = open(app.root_path + "/passwd.cred", "r")
app.config['SQLALCHEMY_DATABASE_URI'] = 'postgresql://' + password.readline().rstrip() + ':' + password.readline().rstrip() + '@localhost/' + password.readline().rstrip()
db = SQLAlchemy(app)
import models


migrate = Migrate(app, db)
SQLALCHEMY_TRACK_MODIFICATIONS = False


@app.route('/')
def landing():
    return app.send_static_file('map.html')

@app.route('/graph')
def graphs():
    time = []
    level = []
    water_temp = []
    air_temp = []
    pressure = []
    humidity = []

    site = models.Deployment.query.filter(models.Deployment.id == request.args.get("deployment")).first()
    readings = models.Measurement.query.filter(models.Measurement.deployment == request.args.get("deployment")).order_by(models.Measurement.time.desc()).all()
    for measurement in readings:
        time.insert(0, measurement.time)
        level.insert(0, site.height - measurement.level)
        water_temp.insert(0, measurement.water_temp)
        air_temp.insert(0, measurement.air_temp)
        pressure.insert(0, measurement.pressure)
        humidity.insert(0, measurement.humidity)

    return render_template('graphs.html', water_level=level, labels=time, air_temp=air_temp, water_temp=water_temp, pressure=pressure, humidity=humidity)

@app.route("/deployments", methods=["GET"])
def deployments():
    sites = models.Deployment.query.all()
    return jsonify([i.__serialize__() for i in sites])

@app.route("/results", methods=["GET"])
def results():
    readings = models.Measurement.query.filter(models.Measurement.deployment == request.args.get("deployment")).all()
    return jsonify([i.__serialize__() for i in readings])

@app.route("/rockblock", methods=["POST"])
def rock_block():
    imei = request.form.get('imei')
    momsn = request.form.get('momsn')
    transmit_time = request.form.get('transmit_time')
    iridium_latitude = request.form.get("iridium_latitude")
    iridium_longitude = request.form.get("iridium_longitude")
    iridum_cep = request.form.get("iridium_cep")
    payload = request.form.get('data')

    print(request.form, file=sys.stdout)
    #deployment = 1 ##TODO Look up database for deployments
    dep = models.Deployment.query.filter_by(imei=imei).order_by(models.Deployment.deployed.desc()).first()
    deployment = dep.id

    period = int(payload[8:12], 16)

    time1 = datetime.utcfromtimestamp(int(payload[0:8], 16))
    level1 = int(payload[12:16], 16)
    water_temp1 = int(payload[16:20], 16)/100.0
    air_temp1 = int(payload[20:24], 16)/100.0
    humidity1 = int(payload[24:28], 16)/100.0
    pressure1 = int(payload[28:32], 16)/1000.0

    time2 = time1 + timedelta(minutes=period)
    level2 = int(payload[32:36], 16)
    water_temp2 = int(payload[36:40], 16) / 100.0
    air_temp2 = int(payload[40:44], 16) / 100.0
    humidity2 = int(payload[44:48], 16) / 100.0
    pressure2 = int(payload[48:52], 16) / 1000.0

    time3 = time2 + timedelta(minutes=period)
    level3 = int(payload[52:56], 16)
    water_temp3 = int(payload[56:60], 16) / 100.0
    air_temp3 = int(payload[60:64], 16) / 100.0
    humidity3 = int(payload[64:68], 16) / 100.0
    pressure3 = int(payload[68:72], 16) / 1000.0

    time4 = time3 + timedelta(minutes=period)
    level4 = int(payload[72:76], 16)
    water_temp4 = int(payload[76:80], 16) / 100.0
    air_temp4 = int(payload[80:84], 16) / 100.0
    humidity4 = int(payload[84:88], 16) / 100.0
    pressure4 = int(payload[88:92], 16) / 1000.0

    entry1 = models.Measurement(deployment, time1, level1, water_temp1, air_temp1, humidity1, pressure1, imei, iridium_latitude, iridium_longitude)
    db.session.add(entry1)

    entry2 = models.Measurement(deployment, time2, level2, water_temp2, air_temp2, humidity2, pressure2, imei, iridium_latitude, iridium_longitude)
    db.session.add(entry2)

    entry3 = models.Measurement(deployment, time3, level3, water_temp3, air_temp3, humidity3, pressure3, imei, iridium_latitude, iridium_longitude)
    db.session.add(entry3)

    entry4 = models.Measurement(deployment, time4, level4, water_temp4, air_temp4, humidity4, pressure4, imei, iridium_latitude, iridium_longitude)
    db.session.add(entry4)

    db.session.commit()

    return Response(status=200, mimetype='application/json')


if __name__ == '__main__':
    app.run()


