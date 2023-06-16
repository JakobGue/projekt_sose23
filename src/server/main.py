import json
import os
import time
from datetime import datetime

import paho.mqtt.client as paho
import pymysql
from dotenv import load_dotenv
from paho import mqtt

load_dotenv()



def connect_db():
    connection = pymysql.connect(
        host=os.environ.get('MYSQL_HOST'),
        port=int(os.environ.get('MYSQL_PORT')),
        user=os.environ.get('MYSQL_USER'),
        password=os.environ.get('MYSQL_PW'),
        db=os.environ.get('MYSQL_DB'),
        charset='utf8mb4',
        cursorclass=pymysql.cursors.DictCursor)
    return connection


def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Connected to MQTT Broker!")
    else:
        print("Failed to connect, return code %d\n", rc)


def on_publish(client, userdata, mid, properties=None):
    print("Published: " + str(mid))


def on_subscribe(client, userdata, mid, granted_qos, properties=None):
    print("Subscribed: " + str(mid) + " " + str(granted_qos))


def on_message(client, userdata, msg):
    data = json.loads(msg.payload.decode())['data']
    timestamp = datetime.utcnow()
    connection = connect_db()
    print(f"[{datetime.utcnow()}] Received `{msg.payload.decode()}` from `{msg.topic}` topic")
    try:
        cursor = connection.cursor()
        for measurement in data:
            exceed_type = None
            query = f"""INSERT INTO measurement_data ("timestamp", pot_id, sensor_id, measurement_type, value) 
                    VALUES (
                        '{timestamp}', 
                        {measurement['pot_id']}, 
                        {measurement['sensor_id']}, 
                        '{measurement['measurement_type']}', 
                        {measurement['value']}
                        )
                    """
            cursor.execute(query)
            mes_id = cursor.lastrowid
            connection.commit()
            query = f"""
                SELECT min_{measurement['measurement_type']}, max_{measurement['measurement_type']} 
                FROM plant_data JOIN pot_data ON plant_data.id = pot_data.plant_id
                WHERE pot_data.id = {measurement['pot_id']}
            """
            cursor.execute(query)
            min_max = cursor.fetchone()
            if  measurement['value'] > min_max[f"max_{measurement['measurement_type']}"]:
                exceed_type = 'max'
            elif measurement['value'] < min_max[f"min_{measurement['measurement_type']}"]:
                exceed_type = 'min'

            if exceed_type:
                print(f"ALERTING: {measurement['measurement_type']} - {exceed_type}")
            
                cursor.execute(f"""
                    INSERT INTO alertings (measurement_id, exceed_type)
                    VALUES ({mes_id}, '{exceed_type}')
                """)
                client.publish(f"alertings_{measurement['pot_id']}", f"{measurement['measurement_type']}-{exceed_type}")
            
    finally:
        connection.close()


def connect_mqtt():
    client = paho.Client(client_id="Raspi", userdata=None, protocol=paho.MQTTv5)
    client.on_connect = on_connect

    client.tls_set(tls_version=mqtt.client.ssl.PROTOCOL_TLS)
    client.username_pw_set(os.environ.get('MQTT_USERNAME'), os.environ.get('MQTT_PASSWORD'))
    client.connect(os.environ.get('MQTT_BROKER'), int(os.environ.get('MQTT_PORT')))


    client.on_subscribe = on_subscribe
    client.on_message = on_message
    client.on_publish = on_publish

    
    client.subscribe(os.environ.get('MQTT_TOPIC'), qos=1)
    return client



if __name__ == '__main__':
    while True:
        try:
            c = connect_mqtt()
            c.loop_forever()
        except Exception as err:
            print(err)
            continue