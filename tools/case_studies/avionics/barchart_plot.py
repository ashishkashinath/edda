import matplotlib.pyplot as plt
import numpy as np

# ── Edit your data here ──────────────────────────────────────────
labels    = [
                "Flt. Ctrl.\n" r"$FC_{11} \rightarrow FC_{41}$", #F1, 
                "Flt. Ctrl.\n" + r"$FC_{12} \rightarrow FC_{42}$", # F2, 
                "Flt. Ctrl.\n" + r"$FC_{13} \rightarrow FC_{31}$", # F3
                "Flt. Ctrl.\n" + r"$FC_{32} \rightarrow FC_{43}$", # F4
                "Ckt. Ctrl.\n" + r"$CC_{11} \rightarrow CC_{21}$", # F5
                "Ckt. Ctrl.\n" + r"$CC_{41} \rightarrow CC_{22}$", # F6
                "Eng. Ctrl.\n" + r"$EC_{11} \rightarrow EC_{31}$", # F7
                "Eng. Ctrl.\n" + r"$EC_{32} \rightarrow EC_{41}$" # F8
            ]
# F1, F2, F3, F4, F5, F6, F7, F8 
series1   = [900, 800, 650, 700, 750, 750, 200, 500]   # Edda numbers
series2   = [1000, 1200, 650, 1150, 750, 750, 200, 500]   # RealFlow numbers

series1_label = "Edda"
series2_label = "RealFlow"

#chart_title = "Pairwise bar chart"
y_label     = r"End-to-End Delay ($\mu$s)"
x_label     = "Flows w/ real-time requirements"
# ─────────────────────────────────────────────────────────────────

x     = np.arange(len(labels))
width = 0.35

fig, ax = plt.subplots(figsize=(24, 8))

bars1 = ax.bar(x - width/2, series1, width, label=series1_label)#, color="#378ADD")
bars2 = ax.bar(x + width/2, series2, width, label=series2_label)#, color="#D85A30")

ax.set_xlabel(x_label, fontsize=32)
ax.set_ylabel(y_label, fontsize=32)
ax.set_xticks(x)
ax.set_xticklabels(labels, fontsize=32, rotation=0, ha='center')
ax.tick_params(axis='y', labelsize=32)
ax.legend(fontsize=28)

# deadline lines
half = width / 2 + 0.05

# Global deadline line at 900 us
ax.axhline(y=900, color='red', linewidth=2, linestyle='--')
ax.text(0.75, 900, r"Deadline = 900 $\mu$s", color='red', fontsize=32, va='bottom', ha='center',
        transform=ax.get_yaxis_transform())

ax.spines[["top", "right", "bottom", "left"]].set_visible(True)
ax.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig("avionics_delays_barchart.pdf", dpi=300, bbox_inches="tight")
#plt.show()