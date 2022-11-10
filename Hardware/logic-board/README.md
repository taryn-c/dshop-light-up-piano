To set up the raspberry pi environment, 2 things needs to be run



1. Web server
  - WebServer.js acts as the intermediary between python application in pi and the react application on the tablet.
  - It is set up to send/receive data from python to react.
  - run WebServer.js using node
        - First, run "npm i" to install all dependencies
        - run app using "node WebServer.js"
    
2. Python app
 - rpi-logic.py
    - This python application handles inputs from the arduinos and sends the input data to the webserver
    - Run using python

