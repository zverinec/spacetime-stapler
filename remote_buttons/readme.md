# The Buttons

To start a server on port 5000:

```
./server.py
```

And go to the URL `http://localhost:5000/` for the main page or
`http://localhost:5000/iamthedoctor.html` for the service page.

The clients have to be run as root (to get access to the keyboard and RPi GPIO):

```
./client.py [-s] serveraddress:port
```

If you specify `-s` it is run in a "soft mode" and it waits for the space key
press.