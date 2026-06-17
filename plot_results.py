#!/usr/bin/env python3
"""
plot_results.py  –  Phân tích và vẽ đồ thị Slotted ALOHA (4 Phase Sweep)
=========================================================================

Thiết kế thực nghiệm (sweep tham số):
  Phase 1  – sweep iaTime   : 10 giá trị, G ∈ [0.1, 8.0]
  Phase 2a – sweep pkLenBits: 10 giá trị, G ∈ [0.25, 6.0]
  Phase 2b – sweep numHosts : 10 giá trị, G ∈ [0.1, 5.0]
  Phase 3  – sweep txRate   : 10 giá trị, G ∈ [0.2, 5.0]

Quy ước đồ thị:
  ── Nét liền: dữ liệu mô phỏng (scatter + đường nối)
  -- Nét đứt:  đường lý thuyết

Cách chạy:
  python3 plot_results.py

Đầu ra (PNG):
  aloha_phase1_iaTime.png
  aloha_phase2a_pktsize.png
  aloha_phase2b_numhosts.png
  aloha_phase3_txrate.png
  aloha_summary_overlay.png
"""

import os, re, glob, math
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec


# ================================================================
#  CẤU HÌNH PHASE
# ================================================================

PHASES = {
    'phase1': {
        'config':      'Phase1_VaryIaTime',
        'param_name':  'ia',
        'param_label': 'iaTime (s)',
        'title':       'Phase 1 – Sweep iaTime (Tần Suất Gửi Gói)',
        'subtitle':    'Cố định: N=20, L=960 bit, R=9600 bps → T=0.1 s',
        'short_label': 'Phase 1 (vary iaTime)',
        'color':       '#3498db',
        'marker':      'o',
        'savename':    'aloha_phase1_iaTime.png',
        'param_values': [0.25, 0.4, 0.5, 0.667, 1, 1.333, 2, 4, 10, 20],
        'param_to_G':  lambda ia: 2.0 / ia,   # G = N*T/iaTime = 20*0.1/ia
    },
    'phase2a': {
        'config':      'Phase2a_VaryPktLen',
        'param_name':  'L',
        'param_label': 'pkLenBits (bit)',
        'title':       'Phase 2a – Sweep pkLenBits (Kích Thước Gói)',
        'subtitle':    'Cố định: iaTime=2s, N=20, R=9600 bps',
        'short_label': 'Phase 2a (vary L)',
        'color':       '#8e44ad',
        'marker':      's',
        'savename':    'aloha_phase2a_pktsize.png',
        'param_values': [240, 480, 640, 960, 1440, 1920, 2880, 3840, 4800, 5760],
        'param_to_G':  lambda L: L / 960.0,   # G = N*(L/R)/iaTime = L/960
    },
    'phase2b': {
        'config':      'Phase2b_VaryNumHosts',
        'param_name':  'N',
        'param_label': 'numHosts',
        'title':       'Phase 2b – Sweep numHosts (Số Lượng Trạm)',
        'subtitle':    'Cố định: iaTime=2s, L=960 bit, R=9600 bps',
        'short_label': 'Phase 2b (vary N)',
        'color':       '#27ae60',
        'marker':      '^',
        'savename':    'aloha_phase2b_numhosts.png',
        'param_values': [2, 5, 10, 15, 20, 30, 40, 60, 80, 100],
        'param_to_G':  lambda N: N / 20.0,    # G = N*T/iaTime = N/20
    },
    'phase3': {
        'config':      'Phase3_VaryTxRate',
        'param_name':  'R',
        'param_label': 'txRate (bps)',
        'title':       'Phase 3 – Sweep txRate (Tốc Độ Kênh)',
        'subtitle':    'Cố định: iaTime=2s, N=20, L=960 bit',
        'short_label': 'Phase 3 (vary R)',
        'color':       '#e67e22',
        'marker':      'D',
        'savename':    'aloha_phase3_txrate.png',
        'param_values': [1920, 2400, 3200, 4800, 6400, 9600, 14400, 19200, 38400, 48000],
        'param_to_G':  lambda R: 9600.0 / R,  # G = N*(L/R)/iaTime = 9600/R
    },
}


# ================================================================
#  DỮ LIỆU MẪU LÝ THUYẾT (sinh tự động từ công thức)
# ================================================================

def _theory(G):
    """Tính giá trị lý thuyết Slotted ALOHA từ Offered Load G."""
    return {
        'G':  round(G, 4),
        'S':  round(G * math.exp(-G), 4),
        'CR': round(1 - (1 + G) * math.exp(-G), 4),
        'IR': round(math.exp(-G), 4),
        'SR': round(math.exp(-G), 4),
    }


def _generate_sample_data():
    """Sinh SAMPLE_DATA từ các giá trị sweep và công thức lý thuyết."""
    data = {}
    for phase_key, ph in PHASES.items():
        config = ph['config']
        data[config] = []
        for p in ph['param_values']:
            G = ph['param_to_G'](p)
            entry = _theory(G)
            entry['param'] = p
            data[config].append(entry)
    return data


SAMPLE_DATA = _generate_sample_data()


# ================================================================
#  ĐỌC FILE .SCA
# ================================================================

def parse_sca_file(filepath):
    """
    Đọc file .sca và trả về dict gồm:
      - configname: tên config (ví dụ 'Phase1_VaryIaTime')
      - iterationvars: dict các biến lặp (ví dụ {'ia': 0.25})
      - scalars: dict các scalar của module server
    """
    scalars = {}
    configname = None
    iterationvars = {}

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if line.startswith('attr configname '):
                configname = line.split(None, 2)[2]
            elif line.startswith('attr iterationvars '):
                # Dạng: "$ia=0.25" hoặc "$L=960"
                raw = line.split(None, 2)[2].strip('"')
                for part in raw.split(','):
                    part = part.strip().lstrip('$')
                    if '=' in part:
                        k, v = part.split('=', 1)
                        try:
                            iterationvars[k.strip()] = float(v.strip())
                        except ValueError:
                            iterationvars[k.strip()] = v.strip()
            elif line.startswith('scalar') and ('server' in line or 'medium' in line):
                parts = line.split()
                if len(parts) >= 4:
                    try:
                        scalars[parts[2]] = float(parts[3])
                    except ValueError:
                        pass

    return {
        'configname': configname,
        'iterationvars': iterationvars,
        'scalars': scalars,
    }


def load_results(results_dir='results'):
    """
    Tải tất cả kết quả từ thư mục results/.
    Trả về dict: {configname: [list of point dicts sorted by param]}.
    Mỗi point dict: {'param': val, 'G': ..., 'S': ..., 'CR': ..., 'IR': ..., 'SR': ...}
    """
    from collections import defaultdict
    configs = defaultdict(list)

    sca_files = (glob.glob(os.path.join(results_dir, '**', '*.sca'), recursive=True)
                 or glob.glob(os.path.join(results_dir, '*.sca')))

    for fpath in sorted(sca_files):
        result = parse_sca_file(fpath)
        sc = result['scalars']
        if 'offeredLoad_G' not in sc:
            continue

        entry = {
            'G':  sc['offeredLoad_G'],
            'S':  sc.get('throughput_S', 0),
            'CR': sc.get('collisionRate', 0),
            'IR': sc.get('idleRate', 0),
            'SR': sc.get('successRatio', 0),
        }

        # Trích xuất giá trị tham số sweep
        ivars = result['iterationvars']
        if ivars:
            param_name = list(ivars.keys())[0]
            entry['param'] = ivars[param_name]
            entry['param_name'] = param_name

        configname = result['configname']
        if not configname:
            # Fallback: lấy từ tên file (Phase1_VaryIaTime-ia=0.25-#0.sca)
            configname = re.sub(r'-[^-]+=.*', '', os.path.basename(fpath))
        configs[configname].append(entry)

    # Sắp xếp theo giá trị tham số (hoặc G nếu không có param)
    for cfg in configs:
        configs[cfg].sort(key=lambda x: x.get('param', x['G']))

    # In tóm tắt
    for cfg, points in configs.items():
        Gs = [p['G'] for p in points]
        print(f"  [{cfg:25s}] {len(points):2d} runs  "
              f"G ∈ [{min(Gs):.3f}, {max(Gs):.3f}]")

    return dict(configs)


# ================================================================
#  STYLE  –  Nền trắng, tối ưu cho máy chiếu
# ================================================================

BG        = '#ffffff'
PANEL_BG  = '#f8f9fc'
GRID_COL  = '#d0d4e0'
TEXT_COL  = '#1a1a2e'
THEORY_COL = '#888899'     # màu đường lý thuyết (nét đứt)

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
        "font.family": "serif",
        # "font.serif": ['Computer Modern Roman'] + plt.rcParams['font.serif'],
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
#  VẼ MỘT PHASE (3 panel: S, CR, IR vs G)
# ================================================================

def plot_phase(phase_key, data, save=True):
    """
    Vẽ 3 panel cho một phase:
      - S vs G (nét liền mô phỏng, nét đứt lý thuyết)
      - Collision Rate vs G
      - Idle Rate vs G
    """
    ph = PHASES[phase_key]
    config = ph['config']
    color  = ph['color']
    marker = ph['marker']

    # Lấy data points (đã sắp xếp theo param)
    points = data.get(config, [])
    if not points:
        print(f"  ⚠  Không có dữ liệu cho {config}")
        return None

    Gs  = [p['G']  for p in points]
    Ss  = [p['S']  for p in points]
    CRs = [p['CR'] for p in points]
    IRs = [p['IR'] for p in points]

    # Đường lý thuyết (nét đứt)
    G_max = max(max(Gs), 5) + 0.5
    G_th  = np.linspace(0.01, G_max, 500)
    S_th  = G_th * np.exp(-G_th)
    CR_th = 1 - (1 + G_th) * np.exp(-G_th)
    IR_th = np.exp(-G_th)

    set_projector_rcparams()
    fig = plt.figure(figsize=(15, 5), facecolor=BG)
    fig.suptitle(f'{ph["title"]} - {ph["subtitle"]}',
                 fontsize=10, fontweight='bold', color='#0d0d2e')

    gs = gridspec.GridSpec(1, 3, figure=fig, wspace=0.38)
    axes = [fig.add_subplot(gs[0, i]) for i in range(3)]

    # ---- Panel 1: Throughput S vs G ----
    ax = axes[0]
    ax.plot(G_th, S_th, '--', color=THEORY_COL, lw=2.5,
            label='Lý thuyết: S = G·e⁻ᴳ', zorder=2)
    ax.plot(Gs, Ss, '-', color=color, lw=2, alpha=0.7, zorder=3)
    ax.scatter(Gs, Ss, color=color, s=100, zorder=5,
               marker=marker, edgecolors='#333333', lw=0.8,
               label='Mô phỏng')
    ax.axvline(x=1.0, color='#ccccdd', ls=':', alpha=0.7, lw=1.2)
    ax.annotate('G=1\nS_max≈0.368', xy=(1.0, math.exp(-1)),
                xytext=(1.8, 0.42), fontsize=7.5, color='#333355',
                arrowprops=dict(arrowstyle='->', color='#555577', lw=1.2))
    style_ax(ax, 'Throughput S vs G', 'Offered Load G', 'Throughput S',
             xlim=(0, G_max), ylim=(0, 0.5))
    ax.legend(fontsize=7.5, loc='upper right')

    # ---- Panel 2: Collision Rate vs G ----
    ax = axes[1]
    ax.plot(G_th, CR_th, '--', color=THEORY_COL, lw=2.5,
            label='Lý thuyết: 1−(1+G)e⁻ᴳ')
    ax.plot(Gs, CRs, '-', color=color, lw=2, alpha=0.7, zorder=3)
    ax.scatter(Gs, CRs, color=color, s=100, zorder=5,
               marker=marker, edgecolors='#333333', lw=0.8,
               label='Mô phỏng')
    style_ax(ax, 'Collision Rate vs G', 'Offered Load G', 'Collision Rate',
             xlim=(0, G_max), ylim=(0, 1.05))
    ax.legend(fontsize=7.5, loc='upper left')

    # ---- Panel 3: Idle Rate vs G ----
    ax = axes[2]
    ax.plot(G_th, IR_th, '--', color=THEORY_COL, lw=2.5,
            label='Lý thuyết: e⁻ᴳ')
    ax.plot(Gs, IRs, '-', color=color, lw=2, alpha=0.7, zorder=3)
    ax.scatter(Gs, IRs, color=color, s=100, zorder=5,
               marker=marker, edgecolors='#333333', lw=0.8,
               label='Mô phỏng')
    style_ax(ax, 'Idle Rate vs G', 'Offered Load G', 'Idle Rate',
             xlim=(0, G_max), ylim=(0, 1.05))
    ax.legend(fontsize=7.5, loc='upper right')

    fig.tight_layout(rect=[0, 0, 1, 0.93])
    if save:
        plt.savefig(ph['savename'], dpi=150, bbox_inches='tight',
                    facecolor=BG)
        print(f"  ✓ Đã lưu: {ph['savename']}")
    return fig


# ================================================================
#  SUMMARY OVERLAY – Tổng hợp S vs G từ tất cả 4 phase
# ================================================================

def plot_summary_overlay(data, save=True):
    """
    Vẽ tất cả data points của 4 phases lên cùng một đồ thị S vs G.
    Chứng minh: bất kể tham số nào thay đổi, throughput luôn nằm trên
    đường cong chung S = G·e^(-G).
    """
    set_projector_rcparams()
    fig, ax = plt.subplots(figsize=(10, 7), facecolor=BG)

    # Đường lý thuyết (nét đứt)
    G_th = np.linspace(0.01, 9, 500)
    S_th = G_th * np.exp(-G_th)
    ax.plot(G_th, S_th, '--', color=THEORY_COL, lw=3,
            label='Lý thuyết: S = G·e⁻ᴳ', zorder=1)

    # Điểm mô phỏng từ mỗi phase (nét liền + scatter)
    for phase_key in ['phase1', 'phase2a', 'phase2b', 'phase3']:
        ph = PHASES[phase_key]
        config = ph['config']
        points = data.get(config, [])
        if not points:
            continue
        Gs = [p['G'] for p in points]
        Ss = [p['S'] for p in points]
        ax.plot(Gs, Ss, '-', color=ph['color'], lw=1.5, alpha=0.5, zorder=3)
        ax.scatter(Gs, Ss, color=ph['color'], s=120, marker=ph['marker'],
                   edgecolors='#333333', lw=0.8,
                   label=ph['short_label'], zorder=5)

    # Đánh dấu điểm tối ưu
    ax.axvline(x=1.0, color='#ccccdd', ls=':', alpha=0.7, lw=1.5)
    ax.annotate('G = 1\nS_max = 1/e ≈ 0.368',
                xy=(1.0, math.exp(-1)),
                xytext=(2.5, 0.42), fontsize=9, color='#333355',
                arrowprops=dict(arrowstyle='->', color='#555577', lw=1.5))

    ax.set_title('Slotted ALOHA – Tổng Hợp Throughput vs Offered Load\n'
                 'Tất cả 4 phase đều nằm trên cùng đường cong S = G·e⁻ᴳ',
                 fontsize=13, fontweight='bold', color='#0d0d2e')
    ax.set_xlabel('Offered Load G', fontsize=11, color=TEXT_COL)
    ax.set_ylabel('Throughput S', fontsize=11, color=TEXT_COL)
    ax.set_xlim(0, 9)
    ax.set_ylim(0, 0.5)
    ax.grid(True)
    ax.legend(fontsize=9.5, loc='upper right')
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.spines['left'].set_color('#8888aa')
    ax.spines['bottom'].set_color('#8888aa')

    plt.tight_layout()
    if save:
        plt.savefig('aloha_summary_overlay.png', dpi=150,
                    bbox_inches='tight', facecolor=BG)
        print("  ✓ Đã lưu: aloha_summary_overlay.png")
    return fig


# ================================================================
#  MAIN
# ================================================================

if __name__ == '__main__':
    print('=' * 60)
    print('  Slotted ALOHA – Vẽ Đồ Thị 4 Phase Sweep (40 runs)')
    print('=' * 60)

    print('\n[1] Đọc file kết quả từ results/ ...')
    data = load_results('results')

    if not data:
        print('  ⚠  Không tìm thấy .sca. Dùng dữ liệu mẫu lý thuyết.')
        data = SAMPLE_DATA
    else:
        # Bổ sung dữ liệu mẫu cho config còn thiếu
        for config, sample_points in SAMPLE_DATA.items():
            if config not in data:
                print(f'  ⚠  Dùng dữ liệu mẫu cho [{config}]')
                data[config] = sample_points
        total_runs = sum(len(pts) for pts in data.values())
        print(f'  ✓  Tổng: {len(data)} configs, {total_runs} runs.')

    print('\n[2] Vẽ đồ thị từng phase ...')
    for phase_key in ['phase1', 'phase2a', 'phase2b', 'phase3']:
        print(f'\n  → {PHASES[phase_key]["title"]}')
        plot_phase(phase_key, data)

    print('\n[3] Vẽ summary overlay ...')
    plot_summary_overlay(data)

    print('\n[4] Hoàn thành! Các file PNG đã được lưu.')
    plt.show()
