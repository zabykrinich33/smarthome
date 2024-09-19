import os
import glob
import time
from influxdb import InfluxDBClient
from datetime import datetime
import requests
import RPi.GPIO as GPIO
import json

url = 'http://192.168.8.10/info'

GPIO.setmode(GPIO.BCM)
# GPIO.setwarnings(False) dont' use this.

GPIO.setup(17, GPIO.OUT)
GPIO.output(17, GPIO.LOW)

fails = 0;

def read_ohmigo_info():
    
    output = requests.get(url)
    if output.status_code == 200:
        result = json.loads(output.text)
        if 'uptime' in result:
            return result['uptime']
        else:
            return 'Unavailable'
        
    else:
        return 'Unavailable'

while True:
    
    influx_db_client = InfluxDBClient('192.168.8.5', 8086, 'home', '12345', 'panna_ohmi_status')
   
    try:
        status = read_ohmigo_info()
        avail = False
        
        if status == 'Unavailable':
            if fails > 10:
                GPIO.output(17, GPIO.LOW)
            else:
                fails = fails + 1
                
            avail = False
        else:
            GPIO.output(17, GPIO.HIGH)
            fails = 0
            avail = True
        
        print(' Status=%s'% status)
        
        timestamp = datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')

        print(timestamp)
        json_body = [
        {
            "measurement": "panna_ohmi_status",
            "tags": {
                "host": "192.168.8.9"
            },
            "time": timestamp,
            "fields": {
                "availability": avail,
            }
        },
        {
            "measurement": "panna_ohmi_uptime",
            "tags": {
                "host": "192.168.8.9"
            },
            "time": timestamp,
            "fields": {
                "uptime": status,
            }
        }
        ]
        
        influx_db_client.write_points(json_body)
        
    except KeyboardInterrupt:  
        GPIO.output(17, GPIO.LOW)
        print("Keyboard interrupt happened!!!") 
    
    except:    
        GPIO.output(17, GPIO.LOW)
        print("Exception happened!!!")

    influx_db_client.close()
    time.sleep(10)
