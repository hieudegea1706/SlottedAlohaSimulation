# Slotted ALOHA Simulation

Mô phỏng giao thức **Slotted ALOHA** bằng **OMNeT++ 6.4.0**, đánh giá hiệu quả sử dụng kênh truyền qua **4 phase sweep** (40 runs) theo phương pháp *ceteris paribus*. Mỗi phase quét 10 giá trị của một tham số duy nhất bằng cú pháp `${...}` của OMNeT++.

---

## Mục Lục

1. [Yêu Cầu Hệ Thống](#1-yêu-cầu-hệ-thống)
2. [Cấu Trúc Project](#2-cấu-trúc-project)
3. [Lý Thuyết Slotted ALOHA](#3-lý-thuyết-slotted-aloha)
4. [Mô Hình Mô Phỏng](#4-mô-hình-mô-phỏng)
5. [Cài Đặt & Build](#5-cài-đặt--build)
6. [Chạy Mô Phỏng](#6-chạy-mô-phỏng)
7. [Thiết Kế Thực Nghiệm — 4 Phase Sweep](#7-thiết-kế-thực-nghiệm--4-phase-sweep)
8. [Kết Quả Mong Đợi](#8-kết-quả-mong-đợi)
9. [Vẽ Đồ Thị](#9-vẽ-đồ-thị)
10. [Kiểm Tra Tính Đúng Đắn](#10-kiểm-tra-tính-đúng-đắn)
11. [Gỡ Lỗi Thường Gặp](#11-gỡ-lỗi-thường-gặp)

---

## 1. Yêu Cầu Hệ Thống

| Công cụ | Phiên bản |
|---------|-----------|
| OMNeT++ | **6.4.0** (bắt buộc) |
| C++ compiler | Clang hoặc GCC ≥ 11 |
| Python | ≥ 3.8 |
| matplotlib | `pip install matplotlib numpy` |

---

## 2. Cấu Trúc Project

```
SlottedAlohaSimulation/
│
├── Host.ned / Host.h / Host.cc       # Module trạm (OOP, sendDirect)
├── Server.ned / Server.h / Server.cc # Module bộ thu trung tâm (OOP, @directIn)
├── SlottedAloha.ned     # Topology mạng (Network)
├── package.ned          # Khai báo package slottedaloha
│
├── omnetpp.ini          # 4 phase sweep (40 runs)
├── plot_results.py      # Script Python vẽ đồ thị kết quả
│
├── Makefile             # Build script (tự động bởi OMNeT++)
├── .project / .cproject # Cấu hình Eclipse/OMNeT++ IDE
├── .oppbuildspec        # Build specification OMNeT++
│
├── results/             # Kết quả mô phỏng (.sca) — bị git ignore
└── out/                 # Build artifacts — bị git ignore
```

---

## 3. Lý Thuyết Slotted ALOHA

### Nguyên lý hoạt động

Slotted ALOHA chia thời gian thành các **slot đồng bộ** có độ dài `T = L/R`:

- `L` = kích thước gói (bit)
- `R` = tốc độ kênh (bps)
- Mỗi trạm chỉ gửi vào **đầu slot** (không gửi giữa chừng)
- Nếu ≥ 2 trạm gửi cùng slot → **collision**, các gói bị hủy

### Công thức tải

```
T (slotTime)    = L / R                        [giây/slot]
λ_host          = 1 / iaTime                   [gói/s mỗi trạm]
λ_total         = N × λ_host                   [gói/s tổng]
G (Offered Load)= λ_total × T = N × T / iaTime [gói/slot]
```

### Công thức hiệu năng lý thuyết

| Chỉ số | Công thức | Ý nghĩa |
|--------|-----------|---------|
| **Throughput** | `S = G · e^(-G)` | Tỉ lệ slot thành công |
| **S tối đa** | `S_max = 1/e ≈ 0.368` tại `G = 1` | Hiệu suất kênh tối ưu |
| **Collision Rate** | `CR = 1 − (1+G)·e^(-G)` | Tỉ lệ slot va chạm |
| **Idle Rate** | `IR = e^(-G)` | Tỉ lệ slot rảnh |
| **Bất biến** | `S + CR + IR = 1` | Tổng luôn bằng 1 |

---

## 4. Mô Hình Mô Phỏng

### Kiến trúc (OOP — sendDirect)

```
omnetpp.ini          Host.cc                       Server.cc
───────────          ────────────────────────      ──────────────────────────
iaTime = 2s    →     exponential(iaTime_mean)      endSlotEvent (mỗi T giây)
                            │                              │
                     getNextTransmissionTime()      ┌───────┴──────────┐
                     = ceil(t / slotTime)       n=0 │ Idle slot         │
                            │                  n=1 │ Success slot  ✓   │
                            │                  n≥2 │ Collision slot ✗  │
                     sendDirect(pk,                │                   │
                       radioDelay,  ───────────────►       │
                       duration,                    recordScalar(G,S,CR,IR)
                       server→gate("in"))           refreshDisplay() (màu icon)
```

### Tham số cơ sở (Baseline)

| Tham số | Giá trị | Ghi chú |
|---------|---------|---------|
| `numHosts` (N) | 20 | Số trạm |
| `pkLenBits` (L) | 960 bit | Kích thước gói |
| `txRate` (R) | 9600 bps | Tốc độ kênh |
| `slotTime` (T) | **0.1 s** | = L/R, tính tự động trong C++ |
| `sim-time-limit` | 3600 s | = 36 000 slot |

> **Quan trọng:** `iaTime` trong `omnetpp.ini` là **hằng số** (mean cố định).  
> Sự ngẫu nhiên nằm trong C++: `scheduleAt(now + exponential(iaTime), arrivalEvent)`.  
> **Không** dùng `exponential()` trong ini — sẽ gây hyper-exponential bug.

### Các scalar được ghi (`recordScalar`)

| Scalar | Mô tả |
|--------|-------|
| `offeredLoad_G` | Offered Load G đo từ mô phỏng |
| `throughput_S` | Throughput S |
| `collisionRate` | Tỉ lệ slot collision |
| `idleRate` | Tỉ lệ slot rảnh |
| `successRatio` | Tỉ lệ gói thành công / tổng gói gửi |
| `throughput_theory` | S = G·e^(-G) tính từ G đo được |
| `cfg_*` | Tham số cấu hình (để Python phân nhóm) |

### Lý Do Chọn Bộ Tham Số Cơ Sở

#### 1. Tại sao N=20, L=960 bit, R=9600 bps?

Ba tham số này không phải tùy ý — chúng được chọn để tạo ra một **điểm vận hành tối ưu sạch** khi `iaTime = 2s`:

```
T = L/R = 960 / 9600 = 0.1 s   (số tròn, dễ kiểm tra)
G = N × T / iaTime = 20 × 0.1 / 2 = 1.0   ← chính xác G_optimal
S = G · e^(-G) = 1 · e^(-1) ≈ 0.368       ← S_max = 1/e
```

Lợi ích: khi bắt đầu từ `G=1.0`, ta có thể tăng/giảm từng tham số một cách đối xứng để khám phá toàn bộ đường cong hiệu năng mà không cần thay đổi baseline code.

#### 2. Tại sao sim-time-limit = 3600s?

```
3600 s ÷ 0.1 s/slot = 36 000 slot
```

Với 36 000 slot, sai số thống kê Monte Carlo thỏa mãn:

```
σ / μ ≈ 1 / √36000 ≈ 0.5%
```

Sai số này nhỏ hơn 1%, đủ để so sánh với lý thuyết mà không cần chạy lại nhiều lần (no repetition needed).

#### 3. Tại sao `SAMPLE_DATA` trong `plot_results.py` có giá trị đó?

`SAMPLE_DATA` được **sinh tự động** từ hàm `_generate_sample_data()` trong script. Hàm này lấy 10 giá trị sweep của mỗi phase, tính G từ công thức mapping (`param_to_G`), rồi áp dụng 4 công thức đóng (*closed-form*):

| Chỉ số | Công thức | Nguồn gốc |
|--------|-----------|-----------|
| `S`  | $G \cdot e^{-G}$ | Xác suất đúng 1 gói trong Poisson($G$) |
| `IR` | $e^{-G}$ | Xác suất 0 gói (slot rảnh) |
| `CR` | $1-(1+G)\cdot e^{-G}$ | Xác suất ≥2 gói (bổ sung) |
| `SR` | $e^{-G}$ | Xác suất thành công mỗi lượt gửi = S/G |

Kiểm chứng nhanh (bất biến bắt buộc):

```
S + CR + IR = Ge^{-G} + [1-(1+G)e^{-G}] + e^{-G} = 1.0  ✓ (mọi G)
```

Bộ dữ liệu này phục vụ hai mục đích:
- **Fallback hoàn chỉnh**: xem đồ thị lý thuyết ngay cả khi chưa chạy mô phỏng.
- **Bổ sung từng phần**: nếu chỉ chạy một số config, các config còn thiếu được điền bằng giá trị lý thuyết.

---

## 5. Cài Đặt & Build

### Clone project

```bash
git clone <repo-url>
cd SlottedAlohaSimulation
```

### Build trong OMNeT++ IDE

1. Mở **OMNeT++ 6.4.0 IDE**
2. **File → Import → Existing Projects into Workspace**
3. Chọn thư mục `SlottedAlohaSimulation/`
4. Nhấn **Ctrl+B** để build

### Build từ terminal (nếu cần)

```bash
# Thiết lập môi trường OMNeT++
source /path/to/omnetpp-6.4.0/setenv

# Build
make MODE=release
```

---

## 6. Chạy Mô Phỏng

### Trong OMNeT++ IDE (khuyến nghị)

1. Mở **Run → Run Configurations...**
2. Tạo **OMNeT++ Simulation** → chọn `omnetpp.ini`
3. Chọn **Config name** (ví dụ: `Phase1_VaryIaTime`)
4. Nhấn **Run** → OMNeT++ tự sinh 10 runs cho các giá trị sweep

### Từ terminal

```bash
# Chạy một phase (OMNeT++ tự chạy 10 giá trị sweep)
./SlottedAlohaSimulation -u Cmdenv -c Phase1_VaryIaTime omnetpp.ini

# Chạy tất cả 4 phase nối tiếp (tổng 40 runs)
for cfg in Phase1_VaryIaTime Phase2a_VaryPktLen \
           Phase2b_VaryNumHosts Phase3_VaryTxRate; do
    echo "=== Running $cfg ==="
    ./SlottedAlohaSimulation -u Cmdenv -c $cfg omnetpp.ini
done
```

Kết quả lưu tại `results/<ConfigName>-<var>=<value>-#0.sca`.

---

## 7. Thiết Kế Thực Nghiệm — 4 Phase Sweep

Phương pháp **sweep tham số** (`${...}` syntax): mỗi phase quét **10 giá trị** của một tham số, giữ nguyên các tham số còn lại. OMNeT++ tự sinh 10 runs cho mỗi config (tổng 40 runs).

```ini
# Ví dụ: Phase 1 quét 10 giá trị iaTime trong 1 config duy nhất
[Config Phase1_VaryIaTime]
*.host[*].iaTime = ${ia=0.25, 0.4, 0.5, 0.667, 1, 1.333, 2, 4, 10, 20}s
# → OMNeT++ tự sinh 10 runs: Phase1_VaryIaTime-ia=0.25-#0.sca, ...
```

### Tổng quan 4 Phase

| Config | Tham số sweep | 10 giá trị | Dải G |
|--------|--------------|------------|-------|
| `Phase1_VaryIaTime` | iaTime (s) | 0.25, 0.4, 0.5, 0.667, 1, 1.333, 2, 4, 10, 20 | 8.0 → 0.1 |
| `Phase2a_VaryPktLen` | L (bit) | 240, 480, 640, 960, 1440, 1920, 2880, 3840, 4800, 5760 | 0.25 → 6.0 |
| `Phase2b_VaryNumHosts` | N | 2, 5, 10, 15, 20, 30, 40, 60, 80, 100 | 0.1 → 5.0 |
| `Phase3_VaryTxRate` | R (bps) | 1920, 2400, 3200, 4800, 6400, 9600, 14400, 19200, 38400, 48000 | 5.0 → 0.2 |

### Công thức G cho mỗi Phase

| Phase | Tham số cố định | Công thức G |
|-------|-----------------|-------------|
| **1** | N=20, L=960b, R=9600bps | G = 2 / iaTime |
| **2a** | iaTime=2s, N=20, R=9600bps | G = L / 960 |
| **2b** | iaTime=2s, L=960b, R=9600bps | G = N / 20 |
| **3** | iaTime=2s, N=20, L=960b | G = 9600 / R |

### Câu hỏi nghiên cứu

| Phase | Biến thiên | Câu hỏi |
|-------|-----------|---------|
| **1** | iaTime (tần suất gửi) | G nào cho throughput tối ưu? |
| **2a** | pkLenBits (kích thước gói) | Gói lớn hơn ảnh hưởng thế nào đến tải kênh? |
| **2b** | numHosts (số trạm) | Mạng đông hơn ảnh hưởng thế nào khi cùng tần suất gửi? |
| **3** | txRate (băng thông) | Tăng băng thông cải thiện hiệu năng thế nào? |

---

## 8. Kết Quả Mong Đợi

Mỗi phase cho ra 10 data points. Tất cả đều phải nằm trên đường cong lý thuyết S = G·e⁻ᴳ. Điểm tối ưu chung: **G = 1.0 → S_max = 1/e ≈ 0.368**.

### Phase 1 — Sweep iaTime (G = 2/iaTime)

| iaTime (s) | G | S | CR | IR |
|---|---|---|---|---|
| 0.25 | 8.000 | 0.003 | 0.997 | 0.000 |
| 0.4 | 5.000 | 0.034 | 0.960 | 0.007 |
| 0.5 | 4.000 | 0.073 | 0.908 | 0.018 |
| 0.667 | 2.999 | 0.149 | 0.801 | 0.050 |
| 1.0 | 2.000 | 0.271 | 0.594 | 0.135 |
| 1.333 | 1.500 | 0.335 | 0.442 | 0.223 |
| **2.0** | **1.000** | **0.368** | **0.264** | **0.368** |
| 4.0 | 0.500 | 0.303 | 0.090 | 0.607 |
| 10.0 | 0.200 | 0.164 | 0.017 | 0.819 |
| 20.0 | 0.100 | 0.090 | 0.005 | 0.905 |

### Phase 2a — Sweep pkLenBits (G = L/960)

| L (bit) | G | S | CR | IR |
|---|---|---|---|---|
| 240 | 0.250 | 0.195 | 0.027 | 0.779 |
| 480 | 0.500 | 0.303 | 0.090 | 0.607 |
| 640 | 0.667 | 0.342 | 0.145 | 0.513 |
| **960** | **1.000** | **0.368** | **0.264** | **0.368** |
| 1440 | 1.500 | 0.335 | 0.442 | 0.223 |
| 1920 | 2.000 | 0.271 | 0.594 | 0.135 |
| 2880 | 3.000 | 0.149 | 0.801 | 0.050 |
| 3840 | 4.000 | 0.073 | 0.908 | 0.018 |
| 4800 | 5.000 | 0.034 | 0.960 | 0.007 |
| 5760 | 6.000 | 0.015 | 0.983 | 0.002 |

### Phase 2b — Sweep numHosts (G = N/20)

| N | G | S | CR | IR |
|---|---|---|---|---|
| 2 | 0.100 | 0.090 | 0.005 | 0.905 |
| 5 | 0.250 | 0.195 | 0.027 | 0.779 |
| 10 | 0.500 | 0.303 | 0.090 | 0.607 |
| 15 | 0.750 | 0.354 | 0.174 | 0.472 |
| **20** | **1.000** | **0.368** | **0.264** | **0.368** |
| 30 | 1.500 | 0.335 | 0.442 | 0.223 |
| 40 | 2.000 | 0.271 | 0.594 | 0.135 |
| 60 | 3.000 | 0.149 | 0.801 | 0.050 |
| 80 | 4.000 | 0.073 | 0.908 | 0.018 |
| 100 | 5.000 | 0.034 | 0.960 | 0.007 |

### Phase 3 — Sweep txRate (G = 9600/R)

| R (bps) | G | S | CR | IR |
|---|---|---|---|---|
| 1920 | 5.000 | 0.034 | 0.960 | 0.007 |
| 2400 | 4.000 | 0.073 | 0.908 | 0.018 |
| 3200 | 3.000 | 0.149 | 0.801 | 0.050 |
| 4800 | 2.000 | 0.271 | 0.594 | 0.135 |
| 6400 | 1.500 | 0.335 | 0.442 | 0.223 |
| **9600** | **1.000** | **0.368** | **0.264** | **0.368** |
| 14400 | 0.667 | 0.342 | 0.145 | 0.513 |
| 19200 | 0.500 | 0.303 | 0.090 | 0.607 |
| 38400 | 0.250 | 0.195 | 0.027 | 0.779 |
| 48000 | 0.200 | 0.164 | 0.017 | 0.819 |

> **Kết luận Phase 3:** Tăng băng thông R↑ → T↓ → G↓ → throughput tốt hơn với **cùng lượng traffic** (iaTime không đổi).

---

## 9. Vẽ Đồ Thị

### Cài đặt dependencies

```bash
pip install matplotlib numpy
```

### Chạy script

```bash
python3 plot_results.py
```

Script tự động đọc tất cả file `.sca` trong `results/` và xuất **5 file PNG**:

| File | Nội dung |
|------|---------|
| `aloha_phase1_iaTime.png` | S, CR, IR vs G — Phase 1 (10 điểm) |
| `aloha_phase2a_pktsize.png` | S, CR, IR vs G — Phase 2a (10 điểm) |
| `aloha_phase2b_numhosts.png` | S, CR, IR vs G — Phase 2b (10 điểm) |
| `aloha_phase3_txrate.png` | S, CR, IR vs G — Phase 3 (10 điểm) |
| `aloha_summary_overlay.png` | Tổng hợp 40 data points trên đường cong S = G·e⁻ᴳ |

### Quy ước đồ thị

| Kiểu nét | Ý nghĩa |
|----------|---------|
| ── **Nét liền** + marker | Dữ liệu mô phỏng (scatter + đường nối) |
| -- **Nét đứt** | Đường lý thuyết |

> Nếu chưa chạy mô phỏng, script tự dùng **dữ liệu mẫu lý thuyết** (sinh tự động từ công thức) để demo đồ thị.

---

## 10. Kiểm Tra Tính Đúng Đắn

Sau mỗi kịch bản, kiểm tra trong console OMNeT++:

```
========== Slotted ALOHA – Kết Quả Mô Phỏng ==========
  slotTime      = 0.1 s          ← phải = L/R = 960/9600
  totalSlots    = 36000          ← phải = sim-time / slotTime
  G (sim)       ≈ G_theory       ← sai số < 5%
  S (sim)       ≈ S (theory)     ← sai số < 5%
  Collision Rate + Idle Rate + Throughput ≈ 1.0
=======================================================
```

**Bất biến quan trọng:**

```
S + CR + IR = 1.0   (tổng 3 loại slot phải bằng tổng slot)
```

---

## 11. Gỡ Lỗi Thường Gặp

| Lỗi | Nguyên nhân | Cách sửa |
|-----|-------------|---------|
| `Parameter 'iaTime' not found` | File `.ned` cũ chưa có `iaTime` | Build lại sau khi sửa `Host.ned` |
| `G_sim ≈ 2 × G_theory` | Dùng `exponential()` trong ini | Đổi thành hằng số: `iaTime = 20s` |
| `slotTime = 0.001s` (sai) | Tham số cũ còn trong ini | `slotTime` tính tự động trong C++, xóa khỏi ini |
| Plot rỗng / không có điểm | Chưa chạy mô phỏng | Chạy 4 phase sweep; script dùng sample data nếu thiếu |
| `G_sim = 0` | `sim-time-limit` quá ngắn | Tăng lên ≥ 1000s để có đủ slot thống kê |
| `.sca` file không được nhận diện | iterationvars bị thiếu | Kiểm tra `attr configname` và `attr iterationvars` trong header .sca |
| Tên config cũ không tìm thấy | Đang dùng config name cũ | Đổi sang: `Phase1_VaryIaTime`, `Phase2a_VaryPktLen`, ... |

---

## Tác Giả

Đồ án học phần **Mạng Máy Tính** - HUST SEEE - Đề tài 1: Mô phỏng Slotted ALOHA bằng OMNeT++ 6.4.0.
