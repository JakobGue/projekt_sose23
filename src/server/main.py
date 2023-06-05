import json
import os
from datetime import datetime

import pymysql
from dotenv import load_dotenv
from paho import mqtt
from paho.mqtt import client as mqtt_client
from fastapi import FastAPI

load_dotenv()

app = FastAPI()


topic = os.environ.get('MQTT_TOPIC')


def connect_mqtt():
    broker = os.environ.get('MQTT_BROKER')
    port = int(os.environ.get('MQTT_PORT'))
    client_id = f'raspi-mqtt-{topic}'
    username = os.environ.get('MQTT_USERNAME')
    password = os.environ.get('MQTT_PASSWORD')
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)
    # Set Connecting Client ID
    client = mqtt_client.Client(client_id)
    client.tls_set(tls_version=mqtt.client.ssl.PROTOCOL_TLS)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client

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

def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        data = json.loads(msg.payload.decode())['data']
        print(f"[{datetime.now()}] Received `{msg.payload.decode()}` from `{msg.topic}` topic")
        connection = connect_db()
        try:
            cursor = connection.cursor()
            for measurement in data:
                query = f"""INSERT INTO measurement_data ("timestamp", pot_id, sensor_id, measurement_type, value) 
                        VALUES (
                            '{datetime.now()}', 
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
                """
                cursor.execute(query)
                min_max = cursor.fetchone()
                if  measurement['value'] > min_max[f"max_{measurement['measurement_type']}"]:
                    exceed_type = 'max'
                elif measurement['value'] < min_max[f"min_{measurement['measurement_type']}"]:
                    exceed_type = 'min'
                else:
                    continue
                cursor.execute(f"""
                    INSERT INTO alertings (measurement_id, exceed_type)
                    VALUES ({mes_id}, '{exceed_type}')
                """)
                
            
        finally:
            connection.close()

    client.subscribe(topic)
    client.on_message = on_message


# def run():
#     client = connect_mqtt()
#     subscribe(client)
#     client.loop_forever()



@app.on_event("startup")
async def startup_event():
    client = connect_mqtt()
    subscribe(client)
    client.loop_start()




@app.get("/health")
async def health():
    return {"status": "ok"}