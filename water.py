from flask import Flask, Response
app = Flask(__name__)


@app.route('/')
def landing():
    return app.send_static_file('map.html')


if __name__ == '__main__':
    app.run()
