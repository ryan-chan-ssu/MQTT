import paho.mqtt.client as mqtt
import mysql.connector
from datetime import datetime

# MQTT Broker Details
BROKER = "7fe480575924421cb99c407d2f8fb733.s1.eu.hivemq.cloud"
PORT = 8883
MQTT_USER = "iot_ryan"
MQTT_PASSWORD = "Wasd1234"
POT_TOPIC = "iot/ryan/potentiometer"  # Potentiometer data topic
SWITCH_TOPIC = "iot/ryan/switch"      # Switch state topic
LED_TOPIC = "iot/ryan/led/control"    # LED control topic

# Hostinger Database Connection Details
DB_HOST = "193.203.166.209"
DB_USER = "u768038663_db_RyanChan"
DB_PASSWORD = "@ndu1nWry4nN"
DB_NAME = "u768038663_RyanChan"

# Connect to the database
def connect_to_database():
    try:
        connection = mysql.connector.connect(
            host=DB_HOST,
            user=DB_USER,
            password=DB_PASSWORD,
            database=DB_NAME
        )
        return connection
    except mysql.connector.Error as err:
        print(f"Database connection failed: {err}")
        return None

# Insert potentiometer data into the database
def push_to_database(data):
    connection = connect_to_database()
    if connection:
        try:
            cursor = connection.cursor()
            query = "INSERT INTO potentiometer_data (value, timestamp) VALUES (%s, %s)"
            cursor.execute(query, data)
            connection.commit()
            print(f"Data inserted into potentiometer_data: {data}")
        except mysql.connector.Error as err:
            print(f"Failed to insert data: {err}")
        finally:
            cursor.close()
            connection.close()

# MQTT Callback: On Connect
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker!")
        client.subscribe([(POT_TOPIC, 0), (SWITCH_TOPIC, 0)])  # Subscribe to both topics
        print(f"Subscribed to: {POT_TOPIC} and {SWITCH_TOPIC}")
    else:
        print(f"Connection failed with code {rc}")

# MQTT Callback: On Message
def on_message(client, userdata, msg):
    try:
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")

        if msg.topic == POT_TOPIC:
            # Handle potentiometer data
            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            pot_value = float(msg.payload.decode().split(":")[1].strip().split(" ")[0])
            push_to_database((pot_value, timestamp))

        elif msg.topic == SWITCH_TOPIC:
            # Handle switch state in real-time (no database storage)
            switch_state = int(msg.payload.decode())
            if switch_state == 1:
                print("[REAL-TIME] Switch Pressed")
            elif switch_state == 0:
                print("[REAL-TIME] Switch Released")

    except Exception as e:
        print(f"Error processing message: {e}")

# Publish LED commands
def control_led(state):
    try:
        payload = str(state)
        client.publish(LED_TOPIC, payload)
        print(f"Published `{payload}` to `{LED_TOPIC}`")
    except Exception as e:
        print(f"Failed to publish to LED topic: {e}")

# MQTT Client Setup
client = mqtt.Client()
client.username_pw_set(MQTT_USER, MQTT_PASSWORD)  # Add authentication
client.tls_set()  # Enable TLS for secure connection
client.on_connect = on_connect
client.on_message = on_message

# Connect to MQTT Broker
print("Connecting to broker...")
client.connect(BROKER, PORT, 60)

# Start the MQTT client loop
try:
    print("Listening for MQTT messages...")
    client.loop_start()

    # Example of controlling LED via Python
    while True:
        cmd = input("Enter `1` to turn LED ON, `0` to turn it OFF, or `exit` to quit: ")
        if cmd == "1":
            control_led(1)
        elif cmd == "0":
            control_led(0)
        elif cmd.lower() == "exit":
            break
        else:
            print("Invalid input. Try again.")

except KeyboardInterrupt:
    print("Stopping program...")
finally:
    client.loop_stop()
    client.disconnect()
