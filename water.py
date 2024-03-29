from flask import Flask, Response, request, render_template, redirect, url_for, flash
import sys
from flask_sqlalchemy import SQLAlchemy
from flask_migrate import Migrate
from datetime import datetime, timedelta
from flask import jsonify
from psycopg2 import IntegrityError
from werkzeug.utils import secure_filename
import os
import json 
import geoalchemy2
from psycopg2.errors import UniqueViolation

import sqlalchemy
import local_config


app = Flask(__name__)

app.debug = True

app.config['SQLALCHEMY_DATABASE_URI'] = 'postgresql://' + local_config.db_user + ":" + local_config.db_password + "@" + local_config.db_server + ":" + local_config.db_port + "/" + local_config.db_name
db = SQLAlchemy(app)
import models

UPLOAD_FOLDER = os.path.join(os.getcwd(), "upload")
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

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
    iridium_cep = request.form.get("iridium_cep")
    payload = request.form.get('data')

    print(request.form, file=sys.stdout)
    #deployment = 1 ##TODO Look up database for deployments
    dep = models.Deployment.query.filter_by(imei=imei).order_by(models.Deployment.deployed.desc()).first()
    deployment = dep.id

    period = int(payload[8:12], 16)

    time1 = datetime.utcfromtimestamp(int(payload[0:8], 16)) - timedelta(minutes=(period / 4)*3)
    level1 = int(payload[12:16], 16)
    water_temp1 = int(payload[16:20], 16) / 100.0
    air_temp1 = int(payload[20:24], 16) / 100.0
    humidity1 = int(payload[24:28], 16) / 100.0
    pressure1 = int(payload[28:32], 16) / 1000.0

    time2 = time1 + timedelta(minutes=(period / 4))
    level2 = int(payload[32:36], 16)
    water_temp2 = int(payload[36:40], 16) / 100.0
    air_temp2 = int(payload[40:44], 16) / 100.0
    humidity2 = int(payload[44:48], 16) / 100.0
    pressure2 = int(payload[48:52], 16) / 1000.0

    time3 = time2 + timedelta(minutes=(period / 4))
    level3 = int(payload[52:56], 16)
    water_temp3 = int(payload[56:60], 16) / 100.0
    air_temp3 = int(payload[60:64], 16) / 100.0
    humidity3 = int(payload[64:68], 16) / 100.0
    pressure3 = int(payload[68:72], 16) / 1000.0

    time4 = time3 + timedelta(minutes=(period / 4))
    level4 = int(payload[72:76], 16)
    water_temp4 = int(payload[76:80], 16) / 100.0
    air_temp4 = int(payload[80:84], 16) / 100.0
    humidity4 = int(payload[84:88], 16) / 100.0
    pressure4 = int(payload[88:92], 16) / 1000.0

    entry1 = models.Measurement(deployment, time1, level1, water_temp1, air_temp1, humidity1, pressure1, imei, iridium_latitude, iridium_longitude, 'satellite')
    db.session.add(entry1)

    entry2 = models.Measurement(deployment, time2, level2, water_temp2, air_temp2, humidity2, pressure2, imei, iridium_latitude, iridium_longitude, 'satellite')
    db.session.add(entry2)

    entry3 = models.Measurement(deployment, time3, level3, water_temp3, air_temp3, humidity3, pressure3, imei, iridium_latitude, iridium_longitude, 'satellite')
    db.session.add(entry3)

    entry4 = models.Measurement(deployment, time4, level4, water_temp4, air_temp4, humidity4, pressure4, imei, iridium_latitude, iridium_longitude, 'satellite')
    db.session.add(entry4)

    db.session.commit()

    return Response(status=200, mimetype='application/json')


@app.route("/import", methods=["GET", "POST"])
def json_import():
    # This runs when the user submits the original form.
    if request.method == "POST":
        # Get deployment ID from the form
        deployment_id = request.form['deployments']
        # TODO: Remember to wrap the filenames in secure_filename
        upload_files_list = request.files.getlist('station-file')
        files_not_added = []

        # Iterate through files from SD Card
        for upload_file in upload_files_list:
            upload_file_path = os.path.join(app.config['UPLOAD_FOLDER'], upload_file.filename)

            if (os.path.exists(upload_file_path)):
                os.remove(upload_file_path)

            upload_file.save(upload_file_path)
            station_file_raw_json = open(upload_file_path, 'r')

            app.logger.info(f"Opening file: {upload_file_path}")

            station_file_data = json.load(station_file_raw_json)

            # psycopg2.errors.UniqueViolation
            try:
                deployment = db.session.query(
                    models.Deployment,
                    models.Deployment.id,
                    models.Deployment.imei,
                    sqlalchemy.func.ST_X(models.Deployment.location).label('latitude'),
                    sqlalchemy.func.ST_Y(models.Deployment.location).label('longitude'),
                ).filter_by(id=deployment_id).first()

                measurement = models.Measurement(deployment_id,
                    datetime.utcfromtimestamp(int(station_file_data['unixTimestamp'])),
                    station_file_data['sonar'], station_file_data['waterTemp'],
                    station_file_data['airTemp'], station_file_data['Humidity'],
                    station_file_data['Pressure'], deployment.imei,
                    deployment.latitude, deployment.longitude, 'import')

                db.session.add(measurement)
                db.session.commit()
                message = f"File successfully uploaded: {upload_files_list}"
            except sqlalchemy.exc.IntegrityError as e:
                db.session.rollback()
                if isinstance(e.orig, UniqueViolation):
                    message = f"Error adding record to database for file: {upload_file.filename}. " \
                    " The record already exists in the database."
                else:
                    message = f"Error adding record to database for file: {upload_file.filename}. " \
                        " There was an integrity contraint violation."
                files_not_added.append(upload_file.filename);

            os.remove(upload_file_path)

        return render_template("import.html", message=message, files_not_added=files_not_added)       
    else:
        deployments = db.session.query(
            models.Deployment,
            models.Deployment.id,
            models.Deployment.imei,
            models.Deployment.location,
            sqlalchemy.func.ST_X(models.Deployment.location).label('latitude'),
            sqlalchemy.func.ST_Y(models.Deployment.location).label('longitude') \
        ).all()

        return render_template("import.html", deployments=deployments)


if __name__ == '__main__':
    app.run()
