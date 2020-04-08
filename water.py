from flask import Flask, Response, request
import sys
app = Flask(__name__)


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
    # f = open("/home/matt/water/test.txt", "a")
    # if imei is not None:
    #     f.write("\nIMEI: ")
    #     f.write(imei)
    # if momsn is not None:
    #     f.write(" momsn: ")
    #     f.write(momsn)
    # if transmit_time is not None:
    #     f.write(" Time: ")
    #     f.write(transmit_time)
    # if iridium_latitude is not None:
    #     f.write("\nlat: ")
    #     f.write(iridium_latitude)
    # if iridium_longitude is not None:
    #     f.write(" Lon: ")
    #     f.write(iridium_longitude)
    # if iridum_cep is not None:
    #     f.write(" CEP: ")
    #     f.write(iridum_cep)
    # if payload is not None:
    #     f.write("\nPayload: ")
    #     f.write(payload)
    #f.write("\n")
    #f.close()
    print(request, file=sys.stdout)

    return Response(status=200, mimetype='application/json')






if __name__ == '__main__':
    app.run()
