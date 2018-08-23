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
    button_deadtime = Every(10)
    status_time = Every(0.5)
    decision_deadline = 0

    if args.soft:
        button = lambda: not keyboard.is_pressed("space")
        reveal = lambda x, _: print("Revealed") if x else None
    else:
        btn = gpiozero.Button(26)
        led_g = gpiozero.LED(16)
        led_r = gpiozero.LED(20)
        def r(x, limit):
            if x:
                led_g.on()
                led_r.off()
            else:
                if timer() < limit:
                    led_g.on()
                    led_r.on()
                else:
                    led_g.off()
                    led_r.on()

        button = lambda: btn.is_pressed
        reveal = r

        for i in range(10):
            l = led_r if i % 2 == 0 else led_g
            l.on()
            time.sleep(0.3)
            l.off()

    while True:
        if status_time.trigger():
            x = requests.get(server + "/status")
            status = x.json()["revealed"]
            reveal(status, decision_deadline)
        if register_timer.trigger():
            requests.post(server + "/register", data={ "id": id } )
        if not button() and button_deadtime.trigger():
            print("Button was pressed!")
            requests.post(server + "/press", data={ "id": id } )
            decision_deadline = timer() + 10
