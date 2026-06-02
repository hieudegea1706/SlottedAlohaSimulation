# Hướng Dẫn Hoàn Thiện Mô Phỏng Slotted ALOHA

## Tóm Tắt Thay Đổi Đã Thực Hiện

Tôi đã refactor toàn bộ dự án từ mô hình xác suất sang Poisson arrivals. Dưới đây là tổng hợp tất cả thay đổi theo từng file.

---

## BƯỚC 1: Build lại trong OMNeT++ IDE

### 1.1 Rebuild project

Sau khi các file đã được cập nhật, trong **OMNeT++ 6.4.0 IDE**:

1. Click chuột phải vào project `SlottedAlohaSimulation` trong **Project Explorer**
2. Chọn **Build Project** (hoặc nhấn `Ctrl+B`)
3. Quan sát **Console** tab ở dưới để xem kết quả build

> **Nếu có lỗi về tham số**: Kiểm tra file `SlottedAloha.ned` đã được lưu chưa.  
> **Nếu có lỗi về `iaTime` không tìm thấy**: Đảm bảo file `Host.cc` mới đã được save.

---

## BƯỚC 2: Chạy 3 Kịch Bản Mô Phỏng

### 2.1 Mở Run Configurations

1. Nhấn nút **Run** (▶) hoặc menu **Run → Run Configurations...**
2. Trong cửa sổ Run Configurations, chọn **OMNeT++ Simulation**
3. Click **New Configuration** (icon tờ giấy + dấu +)

### 2.2 Cấu hình cho LightLoad

```
Name:        SlottedAloha-LightLoad
Project:     SlottedAlohaSimulation  
Ini file(s): omnetpp.ini
Config:      LightLoad
```

Nhấn **Run** → OMNeT++ chạy mô phỏng 3600 giây (= 36.000 slot)

### 2.3 Cấu hình cho MediumLoad

Tạo thêm 1 configuration:
```
Name:   SlottedAloha-MediumLoad
Config: MediumLoad
```

### 2.4 Cấu hình cho HighLoad

```
Name:   SlottedAloha-HighLoad
Config: HighLoad
```

### 2.5 Kết quả mong đợi (console output)

Sau mỗi lần chạy, trong **Qtenv** hoặc **Cmdenv**, bạn sẽ thấy:

```
========== Slotted ALOHA – Kết Quả Mô Phỏng ==========
  slotTime      = 0.1 s
  totalSlots    = 36000
  G (sim)       = 0.099...      ← phải ≈ 0.1 (LightLoad)
  S (sim)       = 0.090...      ← phải ≈ 0.0905
  S (theory)    = 0.090...
  Collision Rate= 0.0045...
  Idle Rate     = 0.9048...
  Success Ratio = 0.905...
=======================================================
```

---

## BƯỚC 3: Xem Scalar Results

### 3.1 Mở Result Files

1. Sau khi chạy xong, vào thư mục `results/` trong Project Explorer
2. Sẽ có các file: `LightLoad-#0.sca`, `MediumLoad-#0.sca`, `HighLoad-#0.sca`
3. Double-click vào file `.sca` để mở trong **Result Analyzer**

### 3.2 Các scalar cần kiểm tra

Trong Result Analyzer, tìm module `SlottedAlohaNetwork.medium`:

| Scalar Name | LightLoad | MediumLoad | HighLoad |
|------------|-----------|------------|---------|
| `offeredLoad_G` | ≈ 0.10 | ≈ 1.00 | ≈ 4.00 |
| `throughput_S` | ≈ 0.090 | ≈ 0.368 | ≈ 0.073 |
| `collisionRate` | ≈ 0.005 | ≈ 0.264 | ≈ 0.908 |
| `idleRate` | ≈ 0.905 | ≈ 0.368 | ≈ 0.018 |
| `throughput_theory` | ≈ 0.090 | ≈ 0.368 | ≈ 0.073 |

---

## BƯỚC 4: Tạo File .anf để Vẽ Trong OMNeT++ IDE

### 4.1 Tạo Analysis File mới

1. Click chuột phải vào project → **New → Analysis File (.anf)**
2. Đặt tên: `SlottedAlohaAnalysis.anf`
3. Click **OK**

### 4.2 Thêm Dataset

Trong **Analysis Editor**:

1. Tab **Inputs**: Click **Add** → chọn tất cả file `*.sca` trong `results/`
2. Tab **Browse Data**: Chọn **Scalars** để xem tất cả scalar

### 4.3 Tạo Chart "Throughput S vs G"

1. Tab **Charts** → Click **Add Chart** → Chọn **Bar Chart** hoặc **Scatter Chart**
2. Trong dialog cấu hình chart:
   - **X axis**: `offeredLoad_G`
   - **Y axis**: `throughput_S`
   - **Group by**: `configname`
3. Đặt tên chart: `Throughput_vs_G`

> **Lưu ý**: Trong OMNeT++ 6.4.0, Analysis Editor sử dụng Python/Matplotlib ở backend.  
> Bạn có thể click **Edit Script** để xem/sửa script Python trực tiếp.

---

## BƯỚC 5: Vẽ Đồ Thị bằng Python (Khuyến nghị)

Script `plot_results.py` đã được tạo sẵn. Chạy như sau:

### 5.1 Cài đặt dependencies

```bash
pip install matplotlib numpy
```

### 5.2 Chạy script

```bash
cd /home/opp_env/default_workspace/SlottedAlohaSimulation
python3 plot_results.py
```

### 5.3 Script sẽ tự động:
- Đọc tất cả file `.sca` trong `results/`
- Trích xuất: `offeredLoad_G`, `throughput_S`, `collisionRate`, `idleRate`
- Vẽ 4 đồ thị (dark theme, publication-quality)
- Lưu file PNG

### 5.4 Nếu chưa có kết quả mô phỏng

Script tự động dùng **dữ liệu mẫu lý thuyết** để demo đồ thị trước.

---

## BƯỚC 6: Phân Tích Kết Quả

### 6.1 So sánh mô phỏng vs lý thuyết

| Kịch bản | G | S (sim) | S = G·e⁻ᴳ | Sai số |
|----------|---|---------|-----------|--------|
| LightLoad | 0.1 | ~0.090 | 0.0905 | <1% |
| MediumLoad | 1.0 | ~0.368 | 0.3679 | <1% |
| HighLoad | 4.0 | ~0.073 | 0.0733 | <2% |

### 6.2 Nhận xét quan trọng

- **G < 1**: Kênh rảnh nhiều (Idle Rate cao), throughput tăng tuyến tính
- **G = 1**: Điểm throughput tối ưu S_max ≈ 0.368 (= 1/e)
- **G > 1**: Va chạm tăng nhanh, throughput giảm mạnh → **overload**
- Mô phỏng Poisson arrivals cho kết quả **sát với lý thuyết** hơn mô hình xác suất p

---

## BƯỚC 7: Gỡ Lỗi Thường Gặp

### Lỗi: "Parameter iaTime not found"
→ Đảm bảo `SlottedAloha.ned` đã có `double iaTime @unit(s)` trong `simple Host`

### Lỗi: "pkLenBits: no such parameter"
→ Kiểm tra `omnetpp.ini` dùng `*.pkLenBits = 960bit` (không phải `packetLength`)

### Lỗi: slotTime = 0.001s (cũ)
→ slotTime bây giờ được **tính tự động** trong C++: `slotTime = pkLenBits / txRate`  
→ **Không còn** nhận từ tham số NED nữa

### G trong kết quả ≈ 0 hoặc rất nhỏ
→ Kiểm tra `sim-time-limit` đủ lớn (ít nhất 1000s để có đủ slot)  
→ Với `iaTime = exponential(20s)`, cần ít nhất 500s để có số liệu ổn định

---

## Tóm Tắt Kiến Trúc Mới

```
omnetpp.ini                Host.cc               Channel.cc
──────────────             ────────────          ──────────────
iaTime = exp(20s)    →     arrivalEvent          endSlotEvent
                           (Poisson timer)       (mỗi 0.1s)
                                │                      │
                           sendEvent             collectPackets
                           (next slot boundary)  ────────────
                                │               n=0 → idle
                           send(pkt, "out") →   n=1 → success
                                                n≥2 → collision
                                                      │
                                              recordScalar(G,S,CR,IR)
```
