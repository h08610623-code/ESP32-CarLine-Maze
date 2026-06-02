# ESP32 Maze Solving Robot

Robot dò line sử dụng ESP32 có khả năng tìm đường trong mê cung dựa trên mô hình đồ thị và điều khiển thông qua giao diện Web.

## Giới thiệu

Dự án xây dựng một robot tự hành có khả năng:

- Dò line bằng cảm biến hồng ngoại
- Di chuyển giữa các nút trong mê cung
- Tính toán đường đi tối ưu
- Điều khiển và giám sát qua trình duyệt Web
- Giao tiếp thời gian thực bằng WebSocket

## Công nghệ sử dụng

### Phần cứng

- ESP32 DevKit V1
- Driver động cơ L298N
- 2 động cơ DC
- 5 cảm biến dò line IR
- Nguồn pin

### Phần mềm

- Arduino Framework
- PlatformIO
- WebServer
- WebSockets
- ArduinoJson

## Cấu hình chân kết nối

### Cảm biến line

| Sensor | GPIO |
|----------|----------|
| S1 | 26 |
| S2 | 25 |
| S3 | 32 |
| S4 | 33 |
| S5 | 27 |

### Driver động cơ

| Chức năng | GPIO |
|------------|------------|
| ENA | 21 |
| IN1 | 22 |
| IN2 | 23 |
| IN3 | 18 |
| IN4 | 19 |
| ENB | 5 |

## Mô hình mê cung

Robot sử dụng đồ thị gồm 20 nút để biểu diễn mê cung.

Mỗi nút được lưu trong ma trận kề và được dùng để tính toán đường đi ngắn nhất từ vị trí hiện tại tới đích.

## Cài đặt

### Clone project

```bash
git clone https://github.com/your_username/ESP32-Maze-Solving-Robot.git
cd ESP32-Maze-Solving-Robot
```

### Cài PlatformIO

Cài đặt:

- Visual Studio Code
- PlatformIO Extension

### Build

```bash
pio run
```

### Upload chương trình

```bash
pio run --target upload
```

### Mở Serial Monitor

```bash
pio device monitor
```

## Cấu hình WiFi

Trong file `CarLine.ino`:

```cpp
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
```

Thay bằng WiFi của bạn rồi upload lại.

## Giao diện Web

Sau khi ESP32 kết nối WiFi:

```text
http://IP_ESP32
```

Giao diện Web cho phép:

- Chọn điểm bắt đầu
- Chọn điểm đích
- Xem đường đi được tính toán
- Điều khiển robot

## Cấu trúc thư mục

```text
.
├── CarLine.ino
├── platformio.ini
├── src/
├── include/
├── lib/
├── test/
├── advanced_feature.py
├── build_ui.py
├── fix_isnode.py
└── fix_turn_bug.py
```
