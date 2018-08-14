#!/usr/bin/env python3

import requests
import argparse
import keyboard
from timeit import default_timer as timer

class Every:
    def __init__(self, period):
        self.period = period
        self.last = 0

    def trigger(self):
        t = timer()
        ret = t - self.last > self.period
        if ret:
            self.last = t
        return ret

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--soft", help="Do not use HW button", action="store_true")
    parser.add_argument("server", help="server address")
    args = parser.parse_args()

    server = "http://" + args.server

    print("Registering... ", end="")
    x = requests.get(server + "/register")
    id = x.json()["id"]
    print("got id: {}".format(id))

    if args.soft:
        button = lambda: keyboard.is_pressed("space")
    else:
        ## ToDo
        button = lambda: False

    register_timer = Every(2)
    button_deadtime = Every(5)
    while True:
        if register_timer.trigger():
            requests.post(server + "/register", data={ "id": id } )
        if button() and button_deadtime.trigger():
            print("Button was pressed!")
            requests.post(server + "/press", data={ "id": id } )
