import paho.mqtt.client as mqtt
import json
from datetime import datetime

# 1. The "Inbox" Callback - This triggers every time ANY message arrives
def on_message(client, userdata, msg):
    # Get the current time for the log
    timestamp = datetime.now().strftime("%H:%M:%S")
    
    # Decode the raw payload from bytes to a standard string
    raw_payload = msg.payload.decode()
    topic = msg.topic
    
    print(f"\n[{timestamp}] MESSAGE RECEIVED")
    print(f"  ├─ Topic:   {topic}")
    
    # Try to format it nicely if it's JSON, otherwise print raw text
    try:
        parsed_json = json.loads(raw_payload)
        pretty_json = json.dumps(parsed_json, indent=4)
        print(f"  └─ Payload:\n{pretty_json}")
    except json.JSONDecodeError:
        print(f"  └─ Payload: {raw_payload}")

# 2. The Connection Callback
def on_connect(client, userdata, flags, reason_code, properties):
    print("Connected to Local Mosquitto Broker!")
    
    # The '#' is the multi-level wildcard. It subscribes to EVERYTHING.
    client.subscribe("#") 
    print("Listening to ALL topics (Wildcard '#' active)...\n")
    print("-" * 50)

# 3. Setup and Run
# Using CallbackAPIVersion.VERSION2 for the latest paho-mqtt standards
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message

print("Starting Master IoT Diagnostic Dashboard...")

try:
    # Connect to your Windows Mosquitto server
    client.connect("localhost", 1883, 60)
    
    # Keep the script running forever
    client.loop_forever()
    
except KeyboardInterrupt:
    print("\nDashboard stopped by user.")
except Exception as e:
    print(f"\nConnection failed: {e}")
    print("Make sure Mosquitto is running in your command prompt!")