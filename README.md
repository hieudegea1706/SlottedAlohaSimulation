# Slotted ALOHA Simulation

Mô phỏng giao thức **Slotted ALOHA** bằng **OMNeT++ 6.4.0**, đánh giá hiệu quả sử dụng kênh truyền dưới 12 kịch bản tải khác nhau theo phương pháp *ceteris paribus*.

---

## Mục Lục

1. [Yêu Cầu Hệ Thống](#1-yêu-cầu-hệ-thống)
2. [Cấu Trúc Project](#2-cấu-trúc-project)
3. [Lý Thuyết Slotted ALOHA](#3-lý-thuyết-slotted-aloha)
4. [Mô Hình Mô Phỏng](#4-mô-hình-mô-phỏng)
5. [Cài Đặt & Build](#5-cài-đặt--build)
6. [Chạy Mô Phỏng](#6-chạy-mô-phỏng)
7. [Thiết Kế Thực Nghiệm — 12 Kịch Bản](#7-thiết-kế-thực-nghiệm--12-kịch-bản)
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
├── Host.cc              # Logic mỗi trạm: Poisson arrivals, gửi gói
├── Channel.cc           # Kênh dùng chung: phân slot, phát hiện collision
├── SlottedAloha.ned     # Topology mạng (NED)
├── package.ned          # Khai báo package slottedaloha
│
├── omnetpp.ini          # 12 kịch bản mô phỏng (4 phase)
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

### Kiến trúc

```
omnetpp.ini          Host.cc                    Channel.cc
───────────          ──────────────────         ──────────────────────
iaTime = 2s    →     arrivalEvent               endSlotEvent (mỗi T giây)
                     (Poisson timer)                    │
                            │                   ┌───────┴──────────┐
                     sendEvent              n=0 │ Idle slot         │
                     (next slot boundary)  n=1 │ Success slot  ✓   │
                            │              n≥2 │ Collision slot ✗  │
                     send(pkt, "out") ─────────►       │
                                                recordScalar(G,S,CR,IR)
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
3. Chọn **Config name** (ví dụ: `LightLoad`)
4. Nhấn **Run**

### Từ terminal

```bash
# Chạy một config
./SlottedAlohaSimulation -u Cmdenv -c LightLoad omnetpp.ini

# Chạy tất cả 12 config nối tiếp
for cfg in LightLoad MediumLoad HighLoad \
           SmallPacket MediumPacket LargePacket \
           FewHosts MediumHosts ManyHosts \
           SlowChannel BaseChannel FastChannel; do
    echo "=== Running $cfg ==="
    ./SlottedAlohaSimulation -u Cmdenv -c $cfg omnetpp.ini
done
```

Kết quả lưu tại `results/<ConfigName>-#0.sca`.

---

## 7. Thiết Kế Thực Nghiệm — 12 Kịch Bản

Phương pháp **ceteris paribus**: mỗi phase chỉ thay đổi **một tham số**, giữ nguyên các tham số còn lại.

```
┌─────────────────────────────────────────────────────────────────┐
│  Phase 1 – Vary iaTime   (N=20, L=960b, R=9600bps, T=0.1s)    │
│    LightLoad  : iaTime=20s   → G=0.1                           │
│    MediumLoad : iaTime=2s    → G=1.0  ← ĐIỂM TỐI ƯU          │
│    HighLoad   : iaTime=0.5s  → G=4.0                           │
├─────────────────────────────────────────────────────────────────┤
│  Phase 2a – Vary pkLenBits  (iaTime=2s, N=20, R=9600bps)      │
│    SmallPacket  : L=480bit  → T=0.05s → G=0.5                 │
│    MediumPacket : L=960bit  → T=0.1s  → G=1.0 (baseline)     │
│    LargePacket  : L=1920bit → T=0.2s  → G=2.0                 │
├─────────────────────────────────────────────────────────────────┤
│  Phase 2b – Vary numHosts   (iaTime=2s, L=960b, R=9600bps)    │
│    FewHosts    : N=5  → G=0.25                                 │
│    MediumHosts : N=20 → G=1.0  (baseline)                     │
│    ManyHosts   : N=40 → G=2.0                                  │
├─────────────────────────────────────────────────────────────────┤
│  Phase 3 – Vary txRate      (iaTime=2s, N=20, L=960bit)       │
│    SlowChannel : R=4800bps  → T=0.2s  → G=2.0                 │
│    BaseChannel : R=9600bps  → T=0.1s  → G=1.0 (baseline)     │
│    FastChannel : R=19200bps → T=0.05s → G=0.5                 │
└─────────────────────────────────────────────────────────────────┘
```

### Câu hỏi nghiên cứu của từng phase

| Phase | Biến thiên | Câu hỏi nghiên cứu |
|-------|-----------|---------------------|
| **1** | iaTime (tần suất gửi) | G nào cho throughput tối ưu? |
| **2a** | pkLenBits (kích thước gói) | Gói lớn hơn ảnh hưởng thế nào đến tải kênh? |
| **2b** | numHosts (số trạm) | Mạng đông hơn ảnh hưởng thế nào khi cùng tần suất gửi? |
| **3** | txRate (băng thông) | Tăng băng thông cải thiện hiệu năng thế nào? |

---

## 8. Kết Quả Mong Đợi

### Phase 1 — Tìm điểm tối ưu

| Config | iaTime | G | S = G·e⁻ᴳ | Idle Rate | Collision Rate |
|--------|--------|---|-----------|-----------|----------------|
| LightLoad | 20 s | 0.10 | 0.090 | 90.5% | 0.5% |
| **MediumLoad** | **2 s** | **1.00** | **0.368** | **36.8%** | **26.4%** |
| HighLoad | 0.5 s | 4.00 | 0.073 | 1.8% | 90.8% |

### Phase 2a — Ảnh hưởng kích thước gói (iaTime=2s, N=20)

| Config | L | T | G | S |
|--------|---|---|---|---|
| SmallPacket | 480 bit | 0.05 s | 0.5 | 0.303 |
| MediumPacket | 960 bit | 0.1 s | 1.0 | 0.368 |
| LargePacket | 1920 bit | 0.2 s | 2.0 | 0.271 |

### Phase 2b — Ảnh hưởng số trạm (iaTime=2s, L=960b)

| Config | N | G | S |
|--------|---|---|---|
| FewHosts | 5 | 0.25 | 0.195 |
| MediumHosts | 20 | 1.0 | 0.368 |
| ManyHosts | 40 | 2.0 | 0.271 |

### Phase 3 — Ảnh hưởng băng thông (iaTime=2s, N=20, L=960b)

| Config | R (bps) | T | G | S |
|--------|---------|---|---|---|
| SlowChannel | 4 800 | 0.2 s | 2.0 | 0.271 |
| BaseChannel | 9 600 | 0.1 s | 1.0 | 0.368 |
| FastChannel | 19 200 | 0.05 s | 0.5 | 0.303 |

> **Kết luận Phase 3:** Tăng băng thông R↑ → T↓ → G↓ → throughput tốt hơn  
> với **cùng lượng traffic** (iaTime không đổi). Đây là lý do tăng băng thông cải thiện hiệu năng mạng ALOHA.

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

Script tự động đọc tất cả file `.sca` trong `results/` và xuất **6 file PNG**:

| File | Nội dung |
|------|---------|
| `aloha_phase1_iaTime.png` | S, Collision Rate, Idle Rate vs G — Phase 1 |
| `aloha_phase2a_pktsize.png` | S, Collision Rate, Idle Rate vs G — Phase 2a |
| `aloha_phase2b_numhosts.png` | S, Collision Rate, Idle Rate vs G — Phase 2b |
| `aloha_phase3_txrate.png` | S, Collision Rate, Idle Rate vs G — Phase 3 |
| `aloha_summary_bar.png` | So sánh S_sim vs S_theory cho 12 config |
| `aloha_phase3_insight.png` | T, G, S theo txRate — insight băng thông |

> Nếu chưa chạy mô phỏng, script tự dùng **dữ liệu mẫu lý thuyết** để demo đồ thị.

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
| `Parameter 'iaTime' not found` | File `.ned` cũ chưa có `iaTime` | Build lại sau khi sửa `SlottedAloha.ned` |
| `G_sim ≈ 2 × G_theory` | Dùng `exponential()` trong ini | Đổi thành hằng số: `iaTime = 20s` |
| `SimTime × SimTime` compile error | Kiểu dữ liệu sai | Dùng `long slotIndex` thay vì `simtime_t` |
| `slotTime = 0.001s` (sai) | Tham số cũ còn trong ini | `slotTime` tính tự động trong C++, xóa khỏi ini |
| Plot rỗng / không có điểm | Chưa chạy đủ config | Chạy tất cả 12 config; script dùng sample data nếu thiếu |
| `G_sim = 0` | `sim-time-limit` quá ngắn | Tăng lên ≥ 1000s để có đủ slot thống kê |

---

## Tác Giả

Đồ án học phần **Mạng Máy Tính** - HUST SEEE - Đề tài 1: Mô phỏng Slotted ALOHA bằng OMNeT++ 6.4.0.
