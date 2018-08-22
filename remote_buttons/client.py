#!/usr/bin/env python3

import requests
import argparse
from timeit import default_timer as timer
import time

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
    parser.add_argument("url", help="url for the button server")
    args = parser.parse_args()

    if args.soft:
        import keyboard
    else:
        import gpiozero

    x = requests.get(args.url)
    server = x.text.strip()
    if not server.startswith("http://"):
        server = "http://" + server

    print("Registering... ", end="")
    x = requests.get(server + "/register")
    id = x.json()["id"]
    print("got id: {}".format(id))

    register_timer = Every(2)
    button_deadtime = Every(5)
    status_time = Every(0.5)

    if args.soft:
        button = lambda: keyboard.is_pressed("space")
        reveal = lambda x: print("Revealed") if x else None
    else:
        btn = gpiozero.Button(26)
        led_g = gpiozero.LED(16)
        led_r = gpiozero.LED(20)
        button = lambda: btn.is_pressed
        reveal = lambda x: led_g.on() if x else led_g.off()
        fail = lambda x: led_r.on() if x else led_r.off()

        led_r.on()
        time.sleep(5)
        led_r.off()

    while True:
        if status_time.trigger():
            x = requests.get(server + "/status")
            status = x.json()["revealed"]
            reveal(status)
        if register_timer.trigger():
            requests.post(server + "/register", data={ "id": id } )
        if not button() and button_deadtime.trigger():
            print("Button was pressed!")
            requests.post(server + "/press", data={ "id": id } )
