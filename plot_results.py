#!/usr/bin/env python3
"""
plot_results.py  –  Phân tích và vẽ đồ thị Slotted ALOHA (12 kịch bản)
========================================================================

Thiết kế thực nghiệm:
  Phase 1  – vary iaTime  : LightLoad, MediumLoad, HighLoad
  Phase 2a – vary L       : SmallPacket, MediumPacket, LargePacket
  Phase 2b – vary N       : FewHosts, MediumHosts, ManyHosts
  Phase 3  – vary R       : SlowChannel, BaseChannel, FastChannel

Cách chạy:
  python3 plot_results.py

Đầu ra (PNG):
  aloha_phase1_iaTime.png
  aloha_phase2a_pktsize.png
  aloha_phase2b_numhosts.png
  aloha_phase3_txrate.png
  aloha_summary_bar.png
"""

import os, re, glob, math
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
from matplotlib.patches import FancyBboxPatch

# ================================================================
#  CẤU HÌNH PHASE
# ================================================================

PHASES = {
    'phase1': {
        'title':    'Phase 1 – Vary iaTime (Tần Suất Gửi Gói)',
        'subtitle': 'Cố định: N=20, L=960 bit, R=9600 bps → T=0.1 s',
        'configs':  ['LightLoad', 'MediumLoad', 'HighLoad'],
        'labels':   ['G≈0.1\n(iaTime=20s)', 'G≈1.0\n(iaTime=2s)', 'G≈4.0\n(iaTime=0.5s)'],
        'colors':   ['#4fc3f7', '#81c784', '#ff8a65'],
        'vary':     'iaTime (s)',
        'vary_vals':['20 s', '2 s', '0.5 s'],
        'savename': 'aloha_phase1_iaTime.png',
    },
    'phase2a': {
        'title':    'Phase 2a – Vary pkLenBits (Kích Thước Gói)',
        'subtitle': 'Cố định: iaTime=2s, N=20, R=9600 bps',
        'configs':  ['SmallPacket', 'MediumPacket', 'LargePacket'],
        'labels':   ['G≈0.5\n(L=480 bit)', 'G≈1.0\n(L=960 bit)', 'G≈2.0\n(L=1920 bit)'],
        'colors':   ['#ce93d8', '#f48fb1', '#ef9a9a'],
        'vary':     'pkLenBits (bit)',
        'vary_vals':['480 bit', '960 bit', '1920 bit'],
        'savename': 'aloha_phase2a_pktsize.png',
    },
    'phase2b': {
        'title':    'Phase 2b – Vary numHosts (Số Lượng Trạm)',
        'subtitle': 'Cố định: iaTime=2s, L=960 bit, R=9600 bps',
        'configs':  ['FewHosts', 'MediumHosts', 'ManyHosts'],
        'labels':   ['G≈0.25\n(N=5)', 'G≈1.0\n(N=20)', 'G≈2.0\n(N=40)'],
        'colors':   ['#80cbc4', '#4db6ac', '#00897b'],
        'vary':     'numHosts',
        'vary_vals':['5', '20', '40'],
        'savename': 'aloha_phase2b_numhosts.png',
    },
    'phase3': {
        'title':    'Phase 3 – Vary txRate (Tốc Độ Kênh / Băng Thông)',
        'subtitle': 'Cố định: iaTime=2s, N=20, L=960 bit',
        'configs':  ['SlowChannel', 'BaseChannel', 'FastChannel'],
        'labels':   ['G≈2.0\n(R=4800 bps)', 'G≈1.0\n(R=9600 bps)', 'G≈0.5\n(R=19200 bps)'],
        'colors':   ['#ffb74d', '#ffd54f', '#fff176'],
        'vary':     'txRate (bps)',
        'vary_vals':['4800', '9600', '19200'],
        'savename': 'aloha_phase3_txrate.png',
    },
}

# Dữ liệu mẫu (dùng khi chưa chạy mô phỏng)
SAMPLE_DATA = {
    # Phase 1
    'LightLoad':    {'G': 0.100, 'S': 0.0905, 'CR': 0.0045, 'IR': 0.9048, 'SR': 0.905},
    'MediumLoad':   {'G': 1.000, 'S': 0.3679, 'CR': 0.2642, 'IR': 0.3679, 'SR': 0.368},
    'HighLoad':     {'G': 4.000, 'S': 0.0733, 'CR': 0.9083, 'IR': 0.0183, 'SR': 0.018},
    # Phase 2a
    'SmallPacket':  {'G': 0.500, 'S': 0.3033, 'CR': 0.0902, 'IR': 0.6065, 'SR': 0.607},
    'MediumPacket': {'G': 1.000, 'S': 0.3679, 'CR': 0.2642, 'IR': 0.3679, 'SR': 0.368},
    'LargePacket':  {'G': 2.000, 'S': 0.2707, 'CR': 0.5940, 'IR': 0.1353, 'SR': 0.135},
    # Phase 2b
    'FewHosts':     {'G': 0.250, 'S': 0.1947, 'CR': 0.0268, 'IR': 0.7788, 'SR': 0.779},
    'MediumHosts':  {'G': 1.000, 'S': 0.3679, 'CR': 0.2642, 'IR': 0.3679, 'SR': 0.368},
    'ManyHosts':    {'G': 2.000, 'S': 0.2707, 'CR': 0.5940, 'IR': 0.1353, 'SR': 0.135},
    # Phase 3
    'SlowChannel':  {'G': 2.000, 'S': 0.2707, 'CR': 0.5940, 'IR': 0.1353, 'SR': 0.135},
    'BaseChannel':  {'G': 1.000, 'S': 0.3679, 'CR': 0.2642, 'IR': 0.3679, 'SR': 0.368},
    'FastChannel':  {'G': 0.500, 'S': 0.3033, 'CR': 0.0902, 'IR': 0.6065, 'SR': 0.607},
}

# ================================================================
#  ĐỌC FILE .SCA
# ================================================================

def parse_sca_file(filepath):
    """Đọc file .sca và trả về dict scalars của module medium."""
    scalars = {}
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if line.startswith('scalar') and 'medium' in line:
                parts = line.split()
                if len(parts) >= 4:
                    scalars[parts[2]] = float(parts[3])
    return scalars


def load_results(results_dir='results'):
    """Tải tất cả kết quả từ thư mục results/."""
    sca_files = (glob.glob(os.path.join(results_dir, '**', '*.sca'), recursive=True)
                 or glob.glob(os.path.join(results_dir, '*.sca')))

    data = {}
    for fpath in sca_files:
        # Tên config từ tên file, ví dụ: LightLoad-#0.sca → LightLoad
        config = re.sub(r'-#\d+\.sca$', '', os.path.basename(fpath))
        sc = parse_sca_file(fpath)
        if 'offeredLoad_G' in sc and 'throughput_S' in sc:
            data[config] = {
                'G':  sc['offeredLoad_G'],
                'S':  sc['throughput_S'],
                'CR': sc.get('collisionRate', 0),
                'IR': sc.get('idleRate', 0),
                'SR': sc.get('successRatio', 0),
            }
            print(f"  [{config:15s}] G={sc['offeredLoad_G']:.4f}  "
                  f"S={sc['throughput_S']:.4f}  "
                  f"CR={sc.get('collisionRate', 0):.4f}  "
                  f"IR={sc.get('idleRate', 0):.4f}")
    return data


# ================================================================
#  STYLE  –  Nền trắng, tối ưu cho máy chiếu
# ================================================================

BG        = '#ffffff'        # nền trắng
PANEL_BG  = '#f8f9fc'        # nền panel (trắng xám nhẹ)
GRID_COL  = '#d0d4e0'        # lưới xám nhạt
TEXT_COL  = '#1a1a2e'        # chữ xanh đậm (contrast cao)
ACCENT    = '#c0392b'        # đường lý thuyết: đỏ đậm rõ trên máy chiếu

def set_projector_rcparams():
    plt.rcParams.update({
        'figure.facecolor': BG,
        'axes.facecolor':   PANEL_BG,
        'axes.edgecolor':   '#8888aa',
        'axes.labelcolor':  TEXT_COL,
        'axes.titlecolor':  '#0d0d2e',
        'xtick.color':      '#333355',
        'ytick.color':      '#333355',
        'grid.color':       GRID_COL,
        'grid.linestyle':   '--',
        'grid.alpha':       0.7,
        'text.color':       TEXT_COL,
        'legend.facecolor': '#ffffff',
        'legend.edgecolor': '#aaaacc',
        'legend.framealpha': 0.95,
        'font.family':      'DejaVu Sans',
        'font.size':        10,
        'axes.linewidth':   1.2,
    })


def style_ax(ax, title, xlabel, ylabel, xlim=None, ylim=None):
    ax.set_title(title, fontsize=11, fontweight='bold', pad=8, color='#0d0d2e')
    ax.set_xlabel(xlabel, fontsize=9.5, color=TEXT_COL)
    ax.set_ylabel(ylabel, fontsize=9.5, color=TEXT_COL)
    ax.grid(True)
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.spines['left'].set_color('#8888aa')
    ax.spines['bottom'].set_color('#8888aa')
    if xlim: ax.set_xlim(xlim)
    if ylim: ax.set_ylim(ylim)


# ================================================================
#  VẼ MỘT PHASE
# ================================================================

def plot_phase(phase_key, data, save=True):
    """
    Vẽ 3 panel cho một phase:
      - S vs G (với đường lý thuyết)
      - Collision Rate vs G
      - Idle Rate vs G
    """
    ph = PHASES[phase_key]
    configs  = ph['configs']
    labels   = ph['labels']
    colors   = ph['colors']

    # Lấy data points theo thứ tự config
    pts = []
    for cfg in configs:
        if cfg in data:
            d = data[cfg]
            pts.append((cfg, d['G'], d['S'], d['CR'], d['IR']))
        else:
            print(f"  ⚠  Thiếu kết quả cho config: {cfg}")

    # Đường lý thuyết
    G_th = np.linspace(0, 5, 500)
    S_th = G_th * np.exp(-G_th)
    CR_th = 1 - (1 + G_th) * np.exp(-G_th)
    IR_th = np.exp(-G_th)

    set_projector_rcparams()
    fig = plt.figure(figsize=(15, 5), facecolor=BG)
    fig.suptitle(f'{ph["title"]}\n{ph["subtitle"]}',
                 fontsize=13, fontweight='bold', color='#0d0d2e', y=1.02)

    gs = gridspec.GridSpec(1, 3, figure=fig, wspace=0.38)
    axes = [fig.add_subplot(gs[0, i]) for i in range(3)]

    # ---- Panel 1: Throughput S vs G ----
    ax = axes[0]
    ax.plot(G_th, S_th, color=ACCENT, lw=2.5, label='S = G·e⁻ᴳ (lý thuyết)', zorder=2)
    ax.axvline(x=1.0, color='#888899', ls=':', alpha=0.7, lw=1.2)
    ax.annotate('G=1\nS_max≈0.368', xy=(1.0, math.exp(-1)),
                xytext=(1.6, 0.42), fontsize=7.5, color='#333355',
                arrowprops=dict(arrowstyle='->', color='#555577', lw=1.2))
    for i, pt in enumerate(pts):
        ax.scatter(pt[1], pt[2], color=colors[i], s=130, zorder=5,
                   marker='o', edgecolors='#333333', lw=1.0,
                   label=labels[i])
    style_ax(ax, 'Throughput S vs G', 'Offered Load G', 'Throughput S', ylim=(0, 0.5))
    ax.legend(fontsize=7.5, loc='upper right')

    # ---- Panel 2: Collision Rate vs G ----
    ax = axes[1]
    ax.plot(G_th, CR_th, color='#b03a2e', lw=2.5, label='1−(1+G)e⁻ᴳ')
    for i, pt in enumerate(pts):
        ax.scatter(pt[1], pt[3], color=colors[i], s=130, zorder=5,
                   marker='s', edgecolors='#333333', lw=1.0,
                   label=labels[i])
    style_ax(ax, 'Collision Rate vs G', 'Offered Load G', 'Collision Rate', ylim=(0, 1.05))
    ax.legend(fontsize=7.5, loc='upper left')

    # ---- Panel 3: Idle Rate vs G ----
    ax = axes[2]
    ax.plot(G_th, IR_th, color='#1a5276', lw=2.5, label='e⁻ᴳ')
    for i, pt in enumerate(pts):
        ax.scatter(pt[1], pt[4], color=colors[i], s=130, zorder=5,
                   marker='^', edgecolors='#333333', lw=1.0,
                   label=labels[i])
    style_ax(ax, 'Idle Rate vs G', 'Offered Load G', 'Idle Rate', ylim=(0, 1.05))
    ax.legend(fontsize=7.5, loc='upper right')

    plt.tight_layout()
    if save:
        plt.savefig(ph['savename'], dpi=150, bbox_inches='tight',
                    facecolor=BG)
        print(f"  ✓ Đã lưu: {ph['savename']}")
    return fig


# ================================================================
#  SUMMARY BAR CHART — So sánh Throughput S_sim vs S_theory
# ================================================================

def plot_summary_bar(data, save=True):
    """Bar chart so sánh S_sim vs S_theory cho tất cả 12 config, nhóm theo phase."""
    set_projector_rcparams()

    # Thứ tự và nhóm
    groups = [
        ('Phase 1\nVary iaTime',   ['LightLoad', 'MediumLoad', 'HighLoad'],   '#3498db'),
        ('Phase 2a\nVary L',       ['SmallPacket', 'MediumPacket', 'LargePacket'], '#8e44ad'),
        ('Phase 2b\nVary N',       ['FewHosts', 'MediumHosts', 'ManyHosts'],   '#27ae60'),
        ('Phase 3\nVary R',        ['SlowChannel', 'BaseChannel', 'FastChannel'], '#e67e22'),
    ]

    all_configs = [c for _, cfgs, _ in groups for c in cfgs]
    G_vals  = [data[c]['G']  if c in data else 0 for c in all_configs]
    S_sim   = [data[c]['S']  if c in data else 0 for c in all_configs]
    S_th    = [g * math.exp(-g) for g in G_vals]

    short_names = [
        'Light', 'Medium', 'High',
        'SmPkt', 'MdPkt', 'LgPkt',
        'FewH', 'MedH', 'ManyH',
        'Slow', 'Base', 'Fast',
    ]

    x = np.arange(len(all_configs))
    bw = 0.35

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 9), facecolor=BG)
    fig.suptitle('Slotted ALOHA – Tổng Hợp Kết Quả 12 Kịch Bản',
                 fontsize=14, fontweight='bold', color='#0d0d2e')

    # --- Subplot 1: S_sim vs S_theory ---
    ax = ax1
    bars_sim = ax.bar(x - bw/2, S_sim, bw, label='S (mô phỏng)', zorder=3,
                      color=[c for _, cfgs, col in groups for c in [col]*len(cfgs)],
                      edgecolor='white', linewidth=0.5, alpha=0.9)
    bars_th  = ax.bar(x + bw/2, S_th,  bw, label='S = G·e⁻ᴳ (lý thuyết)', zorder=3,
                      color='#555577', edgecolor='#aaaacc', linewidth=0.5)

    ax.set_xticks(x)
    ax.set_xticklabels(short_names, fontsize=8.5)
    ax.set_ylabel('Throughput S', fontsize=10, color=TEXT_COL)
    ax.set_title('So Sánh Throughput: Mô Phỏng vs Lý Thuyết', fontsize=11,
                 fontweight='bold', color='#0d0d2e')
    ax.grid(axis='y')
    ax.set_ylim(0, 0.55)
    ax.legend(fontsize=9)
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.spines['left'].set_color('#8888aa')
    ax.spines['bottom'].set_color('#8888aa')

    # Gạch phân nhóm
    for sep in [2.5, 5.5, 8.5]:
        ax.axvline(x=sep, color='#bbbbcc', lw=1.5, ls='--')
    for gx, (gname, _, _) in zip([1, 4, 7, 10], groups):
        ax.text(gx, 0.51, gname, ha='center', fontsize=7.5, color='#555577')

    # --- Subplot 2: Collision + Idle Rate ---
    ax = ax2
    CR_vals = [data[c]['CR'] if c in data else 0 for c in all_configs]
    IR_vals = [data[c]['IR'] if c in data else 0 for c in all_configs]

    ax.bar(x - bw/2, CR_vals, bw, label='Collision Rate', color='#c0392b',
           edgecolor='white', lw=0.5, alpha=0.85, zorder=3)
    ax.bar(x + bw/2, IR_vals, bw, label='Idle Rate',      color='#2980b9',
           edgecolor='white', lw=0.5, alpha=0.85, zorder=3)

    ax.set_xticks(x)
    ax.set_xticklabels(short_names, fontsize=8.5)
    ax.set_ylabel('Rate', fontsize=10, color=TEXT_COL)
    ax.set_title('Collision Rate và Idle Rate theo Kịch Bản', fontsize=11,
                 fontweight='bold', color='#0d0d2e')
    ax.grid(axis='y')
    ax.set_ylim(0, 1.05)
    ax.legend(fontsize=9)
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.spines['left'].set_color('#8888aa')
    ax.spines['bottom'].set_color('#8888aa')

    for sep in [2.5, 5.5, 8.5]:
        ax.axvline(x=sep, color='#bbbbcc', lw=1.5, ls='--')
    for gx, (gname, _, _) in zip([1, 4, 7, 10], groups):
        ax.text(gx, 0.98, gname, ha='center', fontsize=7.5, color='#555577')

    plt.tight_layout()
    if save:
        plt.savefig('aloha_summary_bar.png', dpi=150,
                    bbox_inches='tight', facecolor=BG)
        print("  ✓ Đã lưu: aloha_summary_bar.png")
    return fig


# ================================================================
#  PHASE 3 SPECIAL: S vs txRate (bar chart riêng)
# ================================================================

def plot_phase3_insight(data, save=True):
    """
    Biểu đồ đặc biệt Phase 3:
    Cùng lượng traffic (iaTime=2s, N=20, L=960bit) nhưng txRate khác nhau
    → Thấy rõ: kênh nhanh hơn → G nhỏ hơn → throughput tốt hơn
    """
    cfgs    = ['SlowChannel', 'BaseChannel', 'FastChannel']
    rates   = [4800, 9600, 19200]
    labels  = ['4800 bps\n(Chậm)', '9600 bps\n(Chuẩn)', '19200 bps\n(Nhanh)']
    colors  = ['#e74c3c', '#e67e22', '#27ae60']

    S_sim = [data[c]['S']  if c in data else 0 for c in cfgs]
    G_sim = [data[c]['G']  if c in data else 0 for c in cfgs]
    T_vals = [960/r for r in rates]

    set_projector_rcparams()
    fig, axes = plt.subplots(1, 3, figsize=(13, 4.5), facecolor=BG)
    fig.suptitle('Phase 3 – Ảnh Hưởng Của Băng Thông (txRate)\n'
                 'Cùng lưu lượng: iaTime=2s, N=20, L=960 bit',
                 fontsize=12, fontweight='bold', color='#0d0d2e')

    # Panel 1: T = L/R
    ax = axes[0]
    bars = ax.bar(labels, T_vals, color=colors, edgecolor='#333333', lw=0.8)
    ax.axhline(y=0.1, color='#7f8c8d', ls='--', lw=1.5, label='T=0.1s (chuẩn)')
    style_ax(ax, 'Thời Gian Slot T = L/R', 'txRate', 'T (giây)', ylim=(0, 0.25))
    for b, v in zip(bars, T_vals):
        ax.text(b.get_x() + b.get_width()/2, v + 0.003, f'{v:.3f}s',
                ha='center', fontsize=9, color='white')

    # Panel 2: Offered Load G
    ax = axes[1]
    bars = ax.bar(labels, G_sim, color=colors, edgecolor='#333333', lw=0.8)
    ax.axhline(y=1.0, color='#7f8c8d', ls='--', lw=1.5, label='G=1 (tối ưu)')
    style_ax(ax, 'Offered Load G (đo từ mô phỏng)', 'txRate', 'G', ylim=(0, 2.5))
    ax.legend(fontsize=8)
    for b, v in zip(bars, G_sim):
        ax.text(b.get_x() + b.get_width()/2, v + 0.03, f'{v:.3f}',
                ha='center', fontsize=9, color='white')

    # Panel 3: Throughput S
    ax = axes[2]
    S_th = [g * math.exp(-g) for g in G_sim]
    bars = ax.bar(np.arange(3) - 0.2, S_sim, 0.35, color=colors,
                  edgecolor='#333333', lw=0.8, label='Mô phỏng')
    ax.bar(np.arange(3) + 0.2, S_th, 0.35, color='#bdc3c7',
           edgecolor='#888888', lw=0.8, label='Lý thuyết')
    ax.set_xticks(range(3))
    ax.set_xticklabels(labels, fontsize=8)
    style_ax(ax, 'Throughput S', 'txRate', 'S', ylim=(0, 0.45))
    ax.legend(fontsize=8)
    ax.annotate('Kênh nhanh hơn →\nG nhỏ hơn →\nThroughput tốt hơn',
                xy=(2, S_sim[2] if S_sim[2] > 0 else 0.3),
                xytext=(1.1, 0.38), fontsize=7.5, color='#1a5276',
                arrowprops=dict(arrowstyle='->', color='#1a5276', lw=1.2))

    plt.tight_layout()
    if save:
        plt.savefig('aloha_phase3_insight.png', dpi=150,
                    bbox_inches='tight', facecolor=BG)
        print("  ✓ Đã lưu: aloha_phase3_insight.png")
    return fig


# ================================================================
#  MAIN
# ================================================================

if __name__ == '__main__':
    print('=' * 60)
    print('  Slotted ALOHA – Vẽ Đồ Thị 12 Kịch Bản')
    print('=' * 60)

    print('\n[1] Đọc file kết quả từ results/ ...')
    data = load_results('results')

    if not data:
        print('  ⚠  Không tìm thấy .sca. Dùng dữ liệu mẫu lý thuyết.')
        data = SAMPLE_DATA
    else:
        # Bổ sung dữ liệu mẫu cho config còn thiếu
        for cfg, sample in SAMPLE_DATA.items():
            if cfg not in data:
                print(f'  ⚠  Dùng dữ liệu mẫu cho [{cfg}]')
                data[cfg] = sample
        print(f'  ✓  Tổng: {len(data)} config có kết quả.')

    print('\n[2] Vẽ đồ thị từng phase ...')
    for phase_key in ['phase1', 'phase2a', 'phase2b', 'phase3']:
        print(f'\n  → {PHASES[phase_key]["title"]}')
        plot_phase(phase_key, data)

    print('\n[3] Vẽ summary bar chart ...')
    plot_summary_bar(data)

    print('\n[4] Vẽ Phase 3 insight chart ...')
    plot_phase3_insight(data)

    print('\n[5] Hoàn thành! Các file PNG đã được lưu.')
    plt.show()
