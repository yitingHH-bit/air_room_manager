
Developer:
 jiancai hou  Danila Morozov  （some insigt from Chatgpt）
 for project2 
 
<img width="1223" height="815" alt="image" src="https://github.com/user-attachments/assets/df71cff5-2b01-4012-9a18-f115a728f206" />

protocal
<img width="777" height="367" alt="image" src="https://github.com/user-attachments/assets/914b94cd-1411-4872-a4d9-b45bcef62a2b" />

```` 
# Room Air Temperature & Humidity Monitor

An end-to-end demo project for monitoring room temperature and humidity using:
<img width="1500" height="2000" alt="image" src="https://github.com/user-attachments/assets/ee5eaebb-d237-4fad-a6be-3c1bef16aecc" />
<img width="1500" height="2000" alt="image" src="https://github.com/user-attachments/assets/fb4fb6af-3674-444d-8903-d4136c78a213" />

- **ESP8266** + DHT sensor (temperature & humidity)
- **PlatformIO (VS Code)** for firmware development
- **React.js + Docker** for the web dashboard
- **Firebase Realtime Database** for cloud storage & history


---

## Data Protocol (Device → Cloud / Frontend)

Each measurement is sent as a JSON object with the following fields:

| Field      | Type    | Description                                                                 |
|-----------|---------|-----------------------------------------------------------------------------|
| `device_id` | string  | Unique device ID, e.g. `"esp8266-001"`                                     |
| `ts`        | string  | Timestamp in **UTC**, ISO-8601 format, e.g. `"2025-09-30T07:22:15Z"`       |
| `temp_c`    | number  | Temperature in °C                                                          |
| `rh`        | number  | Relative humidity in percent (0–100)                                       |
| `aqi`       | number/null | Air Quality Index (optional placeholder; use `null` if not available) |

Example payload:

```json
{
  "device_id": "esp8266-001",
  "ts": "2025-11-23T18:36:43Z",
  "temp_c": 25.8,
  "rh": 38.0,
  "aqi": null
}
````

---

## Web Interface

The ESP8266 exposes a simple REST API:

* `GET /api/metrics` – current reading (same JSON as above)
* `POST /api/push` – triggers an immediate cloud upload (to Firebase)
* `GET /api/info` – shows device IP, SSID and ID

The **frontend** is a single-page React app running in a Docker container.
It:

* Polls `/api/metrics` for **live temperature & humidity**
* Calls `/api/push` for manual upload
* Reads measurement history from **Firebase** and displays it in a table

### UI (to be further optimized)
<img width="1223" height="815" alt="image" src="https://github.com/user-attachments/assets/af5cc7c4-866a-4a37-8e70-d8679502e110" />
The React app is embedded in a plain HTML file (no build pipeline) and also exposes an interface slot for future cloud / database features:


Firebase history view:


---

## Project Structure

**Hardware**

* ESP8266 module
* DHT11 (or DHT22) temperature & humidity sensor

**Firmware (VS Code + PlatformIO)**

* Handles Wi-Fi, HTTP server, DHT readings, NTP time sync
* Periodically uploads JSON data to Firebase
* Provides REST API for the React dashboard

**Frontend**

* Pure **React.js** (UMD) in `index.html`
* Uses `fetch()` to talk to ESP8266 and Firebase
* Packaged and served using **Docker + http-server**

**Cloud / Storage**

* **Firebase Realtime Database**
  Path used in this project:

  ```text
  /sensor_readings
      ├─ -NxAbc123... { device_id, ts, temp_c, rh, aqi }
      └─ ...
  ```

Architecture overview:


---

## Docker (Frontend)

Build the Docker image:

```bash
docker build -t username/esp8266-panel-react .
```

Run the container:

```bash
docker run --rm -p 8080:8080 username/esp8266-panel-react
```

Open the dashboard in your browser:

```text
http://localhost:8080
```

> ⚠️ Make sure the `API_BASE` in `web/index.html` points to your ESP8266 IP
> (for example `http://192.168.0.105`).

---

## Firebase Realtime Database
<img width="762" height="387" alt="image" src="https://github.com/user-attachments/assets/0cf57432-3b29-4c09-aafb-8812ea5143be" />

* Database URL example:

  ```text
  https://air-manager-xxxx-default-rtdb.firebaseio.com/
  ```

* Data is written under:

  ```text
  /sensor_readings
  ```

* For testing you can use a permissive rule (do **not** use in production):

  ```json
  {
    "rules": {
      ".read": true,
      ".write": true
    }
  }
  ```

The React UI loads recent history from Firebase and displays it nicely (latest 20 records, sorted by timestamp).

---

## Serial Log (PlatformIO Monitor)

Example output from VS Code / PlatformIO:

```text
--- Terminal on COM3 | 115200 8-N-1
--- Available filters and text transformations: colorize, debug, default, direct, esp8266_exception_decoder, hexlify, log2file, nocontrol, printable, send_on_enter, time
--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H
...
Connected! IP: 192.168.0.105
SoftAP up: SSID=esp8266-setup  PASS=12345678  URL: http://192.168.4.1/
HTTP server started
NTP syncing...
mDNS: http://esp8266.local/
[DHT] T=25.80 °C  H=39.0 %
```

This confirms:

* Station mode IP (`192.168.0.105`) used by the React frontend
* Fallback AP (`esp8266-setup` at `192.168.4.1`)
* HTTP server & NTP sync are working

---

## To-Do / Possible Improvements

* Improve dashboard layout & responsive design
* Add charts for temperature / humidity trends
* Secure Firebase with proper auth & indexed queries
* Support multiple devices (`device_id` filter, per-room view)

```

::contentReference[oaicite:0]{index=0}
` 

