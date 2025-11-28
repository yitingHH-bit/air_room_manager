
---

#  Room Air Temperature & Humidity Monitor

**Developers:** Jiancai Hou, Danila Morozov *(with insights from ChatGPT)*
**Project:** Project 2

---

##  Overview

An end-to-end demo project for monitoring **room temperature** and **humidity**, featuring:

* **ESP8266** + DHT sensor (temperature & humidity)
* **PlatformIO (VS Code)** for firmware development
* **React.js + Docker** for the web dashboard
* **Firebase Realtime Database** for cloud storage & history

<img width="1223" height="815" alt="Dashboard Screenshot" src="https://github.com/user-attachments/assets/df71cff5-2b01-4012-9a18-f115a728f206" />

---

## 📡 Data Protocol (Device → Cloud / Frontend)

Each measurement is sent as a JSON object:

| Field       | Type        | Description                                                           |
| ----------- | ----------- | --------------------------------------------------------------------- |
| `device_id` | string      | Unique device ID, e.g., `"esp8266-001"`                               |
| `ts`        | string      | Timestamp in **UTC**, ISO-8601 format, e.g., `"2025-09-30T07:22:15Z"` |
| `temp_c`    | number      | Temperature in °C                                                     |
| `rh`        | number      | Relative humidity in % (0–100)                                        |
| `aqi`       | number/null | Air Quality Index (optional; `null` if not available)                 |

**Example payload:**

```json
{
  "device_id": "esp8266-001",
  "ts": "2025-11-23T18:36:43Z",
  "temp_c": 25.8,
  "rh": 38.0,
  "aqi": null
}
```

**Protocol Diagram:** <img width="777" height="367" alt="Data Protocol" src="https://github.com/user-attachments/assets/914b94cd-1411-4872-a4d9-b45bcef62a2b" />

---

##  Hardware

* **ESP8266** module
* **DHT11/DHT22** temperature & humidity sensor

**Wiring & Setup:**

<img width="1500" height="2000" alt="Hardware Diagram 1" src="https://github.com/user-attachments/assets/ee5eaebb-d237-4fad-a6be-3c1bef16aecc" />
<img width="1500" height="2000" alt="Hardware Diagram 2" src="https://github.com/user-attachments/assets/fb4fb6af-3674-444d-8903-d4136c78a213" />

---

##  Web Interface

The **ESP8266** exposes a simple REST API:

| Endpoint       | Method | Description                    |
| -------------- | ------ | ------------------------------ |
| `/api/metrics` | GET    | Current reading (JSON)         |
| `/api/push`    | POST   | Trigger immediate cloud upload |
| `/api/info`    | GET    | Device IP, SSID, and ID        |

**React Dashboard:**

* Polls `/api/metrics` for **live readings**
* Calls `/api/push` for manual upload
* Reads measurement **history from Firebase**

<img width="1223" height="815" alt="React Dashboard" src="https://github.com/user-attachments/assets/af5cc7c4-866a-4a37-8e70-d8679502e110" />

---

##  Project Structure

### Hardware

* ESP8266 module
* DHT11/DHT22 sensor

### Firmware (VS Code + PlatformIO)

* Wi-Fi and HTTP server
* DHT sensor readings
* NTP time sync
* Periodic upload to Firebase
* REST API for React dashboard

### Frontend

* **React.js** embedded in `index.html`
* Uses `fetch()` to communicate with ESP8266 and Firebase
* Served via **Docker + http-server**

### Cloud / Storage

**Firebase Realtime Database** path:

```text
/sensor_readings
    ├─ -NxAbc123... { device_id, ts, temp_c, rh, aqi }
```

<img width="762" height="387" alt="Firebase DB" src="https://github.com/user-attachments/assets/0cf57432-3b29-4c09-aafb-8812ea5143be" />

---

##  Docker (Frontend)

**Build Docker image:**

```bash
docker build -t username/esp8266-panel-react .
```

**Run container:**

```bash
docker run --rm -p 8080:8080 username/esp8266-panel-react
```

**Open dashboard:**
`http://localhost:8080`

>  Make sure `API_BASE` in `web/index.html` points to your ESP8266 IP, e.g., `http://192.168.0.105`.

---

##  Serial Log (PlatformIO Monitor)

```text
Connected! IP: 192.168.0.105
SoftAP up: SSID=esp8266-setup  PASS=12345678
HTTP server started
NTP syncing...
[DHT] T=25.80 °C  H=39.0 %
```

Confirms:

* Station mode IP used by React frontend
* Fallback AP for setup
* HTTP server & NTP working

---

##  To-Do / Improvements

* Responsive dashboard design
* Charting temperature & humidity trends
* Secure Firebase with authentication & rules
* Support multiple devices / per-room views



Do you want me to do that next?
