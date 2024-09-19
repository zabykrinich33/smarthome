import os
import glob
import time
from influxdb import InfluxDBClient
from datetime import datetime

os.system('modprobe w1-gpio')
os.system('modprobe w1-therm')

base_dir = '/sys/bus/w1/devices/'

#device_folder0 = glob.glob(base_dir + '28*')[0]
#device_folder1 = glob.glob(base_dir + '28*')[1]

device_folder_in = base_dir + '28-00000fad9eb3';
device_folder_out = base_dir + '28-00000fae4a35';

device_file_in = device_folder_in + '/w1_slave'
device_file_out = device_folder_out + '/w1_slave'

def read_rom_in():
    name_file_in = device_folder_in + '/name'
    f = open(name_file_in, 'r')
    return f.readline()

def read_rom_out():
    name_file_out = device_folder_out + '/name'
    g = open(name_file_out, 'r')
    return g.readline()

def read_temp_raw_in():
    f = open(device_file_in, 'r')
    lines_in = f.readlines()
    f.close()
    return lines_in

def read_temp_raw_out():
    g = open(device_file_out, 'r')
    lines_out = g.readlines()
    g.close()
    return lines_out

def read_temp_in():
    lines_in = read_temp_raw_in()
    while lines_in[0].strip()[-3:] != 'YES':
        lines_in = read_temp_raw_in()
    equals_pos_in = lines_in[1].find('t=')
    temp_string_in = lines_in[1][equals_pos_in + 2:]
    temp_c_in = float(temp_string_in) / 1000.0
    return temp_c_in

def read_temp_out():
    lines_out = read_temp_raw_out()
    while lines_out[0].strip()[-3:] != 'YES':
        lines_out = read_temp_raw_out()
    equals_pos_out = lines_out[1].find('t=')
    temp_string_out = lines_out[1][equals_pos_out + 2:]
    temp_c_out = float(temp_string_out) / 1000.0
    return temp_c_out

while True:
    
    influx_db_client = InfluxDBClient('192.168.8.5', 8086, 'home', '12345', 'panna_temp_in_out')
   
    try:
        temperature_in = read_temp_in()
        temperature_out = read_temp_out()
        
        print(' C_IN=%3.3f'% temperature_in)
        print(' C_OUT=%3.3f'% temperature_out)

        timestamp = datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')

        print(timestamp)
        json_body = [
        {
            "measurement": "panna_temp_in",
            "tags": {
                "host": "192.168.8.7",
                "direction": "in"
            },
            "time": timestamp,
            "fields": {
                "temperature": float(temperature_in),
            }
        },
        {
            "measurement": "panna_temp_out",
            "tags": {
                "host": "192.168.8.7",
                "direction": "out"
            },
            "time": timestamp,
            "fields": {
                "temperature": float(temperature_out),
            }
        }
        ]
        
        influx_db_client.write_points(json_body)
        
    except:    
        print("Exception happened!!!")
   

 
    
#    print(' C_IN=%3.3f'% read_temp_in())
#    print(' C_OUT=%3.3f'% read_temp_out())
#    read_temp0()
#    read_temp1()
#    print('   ')
    influx_db_client.close()
    time.sleep(1)
