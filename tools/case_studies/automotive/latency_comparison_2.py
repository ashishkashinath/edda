import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# ── Parse deadline strings to microseconds ───────────────────────
def parse_deadline(deadline_str):
    if deadline_str.endswith('us'):
        return float(deadline_str[:-2])
    elif deadline_str.endswith('ms'):
        return float(deadline_str[:-2]) * 1000
    elif deadline_str.endswith('s'):
        return float(deadline_str[:-1]) * 1000000
    return float(deadline_str)

# ── Read data ────────────────────────────────────────────────────
df = pd.read_csv('latency_results_1.csv')

categories = df['FlowName'].tolist()

rta_vals = pd.to_numeric(df['Expected-Delay-RTA'].apply(parse_deadline)).to_numpy(dtype=float)
dct_vals = pd.to_numeric(df['Expected-Delay-FRC'].apply(parse_deadline)).to_numpy(dtype=float)

rta = rta_vals 
dct = dct_vals 

# ── Plot setup ───────────────────────────────────────────────────
x     = np.arange(len(categories))
width = 0.35

fig, ax = plt.subplots(figsize=(10, 6))

bars2 = ax.bar(x - width/2, dct, width, label="Edda")#, color="#378ADD")
bars1 = ax.bar(x + width/2, rta, width, label="RealFlow")#, color="#D85A30")


# ── Value labels on top of each bar ─────────────────────────────
'''
for bar, val in zip(bars1, rta):
    ax.text(bar.get_x() + bar.get_width() / 2, val,
            f"{val:.2f}", ha="center", va="bottom",
            fontsize=20, fontweight="bold")

for bar, val in zip(bars2, dct):
    ax.text(bar.get_x() + bar.get_width() / 2, val,
            f"{val:.2f}", ha="center", va="bottom",
            fontsize=20, fontweight="bold")
'''
# ── Axes labels & ticks ──────────────────────────────────────────
ax.set_xlabel("Command and Control Flows", fontsize=32)
ax.set_ylabel(r"End-to-End Delay($\mu$s)", fontsize=32)
ax.set_xticks(x)
#ax.set_xticklabels(categories, fontsize=28)
ax.set_xticklabels([], fontsize=32)
ax.tick_params(axis='y', labelsize=32)
ax.legend(fontsize=28, ncol=2, loc='upper left', bbox_to_anchor=(0.01, 1.04), frameon=True)

# ── Spines & grid (matching first script) ───────────────────────
ax.spines[["top", "right", "bottom", "left"]].set_visible(True)
ax.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig("latency_comparison_2.pdf", dpi=300, bbox_inches="tight")