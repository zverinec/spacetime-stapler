#!/usr/bin/env python3

from flask import Flask, request, send_from_directory, jsonify
from timeit import default_timer as timer
import itertools

app = Flask(__name__, static_url_path='')

def iter_ids():
    from string import ascii_uppercase
    for size in itertools.count(1):
        for s in itertools.product(ascii_uppercase, repeat=size):
            yield "".join(s)

alive_tolerance = 5
show_tolerance = 5

app.clients = {}
app.secret = "The precious secret"
app.id_prefix = str(int(timer())) + "_"
app.ids = iter_ids()
app.revealed = False
app.hide_time = 0
app.press_tolerance = 3

class Client:
    def __init__(self):
        self.on_register()
        self.last_press = 0

    def on_register(self):
        self.last_active = timer()

    def on_press(self):
        self.last_press = timer()

    def active(self):
        return timer() - self.last_active < alive_tolerance

    def is_pressed(self):
        return timer() - self.last_press < app.press_tolerance

def active_count(clients):
    return len(list(filter(lambda x: x[1].active(), clients.items())))

def remove_inactive(clients):
    c = {}
    for id, client in clients.items():
        if client.active():
            c[id] = client
    return c

def is_revealed(clients):
    if len(clients) == 0:
        return False
    clients = remove_inactive(clients)
    return all(c.is_pressed() for c in clients.values())

@app.route('/')
def root():
    return app.send_static_file('index.html')

@app.route('/<path:path>')
def send_web(path):
    return send_from_directory('.', path)

@app.route('/register', methods = ['POST', 'GET'])
def register():
    """
        Client makes a GET to obtain { "id": <id_to_use> }
        Client makes a POST?id=<id> to register.
        Posts every 2 seconds
    """
    if request.method == 'GET':
        id = app.id_prefix + next(app.ids)
        resp = jsonify({ "id": id })
        resp.status_code = 200
        return resp
    id = request.form["id"]
    if id not in app.clients:
        app.clients[id] = Client()
    app.clients[id].on_register()

    resp = jsonify({})
    resp.status_code = 200
    return resp

@app.route('/status', methods = ['GET'])
def status():
    """
        GET returns JSON {
            revealed: true/false,
            clients: <number of registered clients>,
            secret: <string>
        }
    """
    if timer() > app.hide_time:
        app.revealed = False
    if not app.revealed:
        app.revealed = is_revealed(app.clients)
        if app.revealed:
            app.hide_time = timer() + show_tolerance
    data = {
        "revealed": app.revealed,
        "clients": active_count(app.clients),
        "secret": app.secret
    }
    resp = jsonify(data)
    resp.status_code = 200
    return resp

@app.route('/press', methods = ['POST'])
def press():
    """
        POST?id=<id> address when a button is pressed
    """
    id = request.form["id"]
    if id not in app.clients:
        app.clients[id] = Client()
    app.clients[id].on_press()

    resp = jsonify({})
    resp.status_code = 200
    return resp


@app.route('/secret', methods = ['POST'])
def secret():
    """
        POST?secret=<your secret>
    """
    print(request.form)
    print("Here")
    for key, value in request.form.items():
        print(key, value)
    print("There")
    app.secret = request.form["secret"]
    app.clients = {}
    app.revealed = False

    resp = jsonify({})
    resp.status_code = 200
    return resp

@app.route('/press_tolerance', methods = ['POST', 'GET'])
def press_tolerance():
    """
        Client makes a GET to obtain { "press_tolerance": <tolerance> }
        Client makes a POST?press_tolerance=<tolerance> to set it
        Posts every 2 seconds
    """
    if request.method == 'GET':
        resp = jsonify({ "press_tolerance": app.press_tolerance })
        resp.status_code = 200
        return resp
    app.press_tolerance = int(request.form["press_tolerance"])

    resp = jsonify({})
    resp.status_code = 200
    return resp

if __name__ == "__main__":
    app.run(host='0.0.0.0')