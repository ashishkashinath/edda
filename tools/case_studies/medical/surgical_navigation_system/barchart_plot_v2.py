import matplotlib.pyplot as plt
import numpy as np

# ── Edit your data here ──────────────────────────────────────────
labels    = [r"Pat. Mon $\rightarrow$ Endo.", #F1, F3, F4, F6, F7, F9
                r"Endo $\rightarrow$ Pat. Mon", # F2, F5, F8
                r"Foot Pedal $\rightarrow$ HF", # F10
                r"HF $\rightarrow$ Foot Pedal", # F11
                r"HF $\rightarrow$ PUMPE", # F12, F14
                r"PUMPE $\rightarrow$ HF", # F13
                r"Pat. Mon $\rightarrow$ PUMPE", # F15, F17
                r"PUMPE $\rightarrow$ Pat. Mon" # F16
            ]
# F1, F2, F10, F11, F14, F13, F15, F16 
series1   = [0.3, 0.061, 0.071, 0.106, 0.085, 0.112, 0.061, 0.076]   # Edda numbers
series2   = [0.3, 0.911, 0.071, 0.106, 0.085, 2.428, 0.061, 0.076]   # RealFlow numbers

series1_label = "Edda"
series2_label = "RealFlow"

#chart_title = "Pairwise bar chart"
y_label     = "End-to-End Delay (ms)"
x_label     = "Flows"
# ─────────────────────────────────────────────────────────────────

x     = np.arange(len(labels))
width = 0.35

fig, ax = plt.subplots(figsize=(24, 8))

bars1 = ax.bar(x - width/2, series1, width, label=series1_label, color="#378ADD")
bars2 = ax.bar(x + width/2, series2, width, label=series2_label, color="#D85A30")

ax.set_xlabel(x_label, fontsize=28)
ax.set_ylabel(y_label, fontsize=28)
ax.set_xticks(x)
ax.set_xticklabels(labels, fontsize=28, rotation=15, ha='center')
ax.tick_params(axis='y', labelsize=28)
ax.legend(fontsize=24)


# deadline lines
half = width / 2 + 0.05

ax.hlines(0.5, x[1] - width, x[1] + width, colors='k', linewidth=1, linestyles='-')
ax.hlines(2.0, x[5] - width, x[5] + width, colors='k', linewidth=1, linestyles='-')

ax.annotate("Deadline", xy=(x[1] - half, 0.5),
            xytext=(x[1] - half - 1.0, 0.5 + 0.2),
            fontsize=24, color='red',
            arrowprops=dict(arrowstyle="->", color='red', lw=1.5))

ax.annotate("Deadline", xy=(x[5] - half, 2.0),
            xytext=(x[5] - half - 1.0, 2.0 + 0.2),
            fontsize=24, color='red',
            arrowprops=dict(arrowstyle="->", color='red', lw=1.5))

ax.spines[["top", "right", "bottom", "left"]].set_visible(True)
ax.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig("medical_device_ensemble.pdf", dpi=300, bbox_inches="tight")
#plt.show()