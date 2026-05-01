import matplotlib.pyplot as plt
import numpy as np

# ── Edit your data here ──────────────────────────────────────────
labels    = ["Foot Pedal\nControl", "Cam 1", "Cam 2", "Patient\nSensor"]
series1   = [0.3, 10.769, 13.436, 18.053]   # Edda numbers
series2   = [0.3, 11.069, 16.403, 37.006]   # RealFlow numbers

series1_label = "Edda"
series2_label = "RealFlow"

#chart_title = "Pairwise bar chart"
y_label     = "End-to-End Delay (ms)"
x_label     = "Flows"
# ─────────────────────────────────────────────────────────────────

x     = np.arange(len(labels))
width = 0.35

fig, ax = plt.subplots(figsize=(10, 6))

bars1 = ax.bar(x - width/2, series1, width, label=series1_label, color="#378ADD")
bars2 = ax.bar(x + width/2, series2, width, label=series2_label, color="#D85A30")

ax.set_xlabel(x_label, fontsize=28)
ax.set_ylabel(y_label, fontsize=28)
ax.set_xticks(x)
ax.set_xticklabels(labels, fontsize=28)
ax.tick_params(axis='y', labelsize=28)
ax.legend(fontsize=24)


#ax.bar_label(bars1, padding=3, fontsize=18)
#ax.bar_label(bars2, padding=3, fontsize=18)

for b1, b2 in zip(bars1, bars2):
    v1, v2 = b1.get_height(), b2.get_height()
    if v1 > 0:
        pct = (v2 - v1) / v1 * 100
        if pct > 15:
            taller = b2 if v2 >= v1 else b1
            shorter_h = min(v1, v2)
            taller_h  = max(v1, v2)
            y_mid = shorter_h + (taller_h - shorter_h) / 2
            x_left = taller.get_x() - 0.02

            ### Extra annotation on extra percentages
            '''
            ax.annotate("", xy=(x_left, shorter_h), xytext=(x_left, taller_h),
                            arrowprops=dict(arrowstyle="<->", color="#D85A30",
                                        lw=1.5, mutation_scale=10))

            ax.text(x_left - 0.02, y_mid,
                    f"+{pct:.1f}%", ha='right', va='center', fontsize=18,
                    color="#D85A30")
            '''

# deadline lines
cam2_x      = x[2]
patient_x   = x[3]
half         = width / 2 + 0.05

ax.hlines(15, cam2_x - width, cam2_x + width, colors='k', linewidth=1, linestyles='-')
ax.hlines(25, patient_x-width, patient_x + width, colors='k', linewidth=1, linestyles='-')

ax.annotate("Deadline", xy=(cam2_x - half, 15),
            xytext=(cam2_x - half - 1.0, 15 + 3),
            fontsize=24, color='red',
            arrowprops=dict(arrowstyle="->", color='red', lw=1.5))

ax.annotate("Deadline", xy=(patient_x - half, 25),
            xytext=(patient_x - half - 1.0, 25 + 3),
            fontsize=24, color='red',
            arrowprops=dict(arrowstyle="->", color='red', lw=1.5))

ax.spines[["top", "right", "bottom", "left"]].set_visible(True)
ax.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig("operating_room.pdf", dpi=300, bbox_inches="tight")
#plt.show()