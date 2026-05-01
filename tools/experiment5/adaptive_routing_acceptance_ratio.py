import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from io import StringIO

# Your CSV data
data = """Scale,Deadline,DCTAcceptanceRatio,RealFlowAcceptanceRatio
1.0,100.000us,0.30392,0.1864
1.0,200.000us,0.49304,0.31928
1.0,300.000us,0.56848,0.39664
1.0,400.000us,0.63296,0.48736
1.0,500.000us,0.69368,0.55096
0.9,100.000us,0.33152,0.17824
0.9,200.000us,0.5096,0.31024
0.9,300.000us,0.57696,0.38936
0.9,400.000us,0.64432,0.47984
0.9,500.000us,0.71096,0.54456
0.8,100.000us,0.3584,0.17088
0.8,200.000us,0.52104,0.304
0.8,300.000us,0.59016,0.37936
0.8,400.000us,0.66,0.4696
0.8,500.000us,0.73,0.5384
0.6,100.000us,0.4392,0.15584
0.6,200.000us,0.54272,0.2824
0.6,300.000us,0.61744,0.35872
0.6,400.000us,0.69536,0.4492
0.6,500.000us,0.77248,0.52344
0.5,100.000us,0.466,0.15056
0.5,200.000us,0.55456,0.2716
0.5,300.000us,0.63384,0.34944
0.5,400.000us,0.71408,0.44032
0.5,500.000us,0.79584,0.51448
0.2,100.000us,0.49392,0.12752
0.2,200.000us,0.5996,0.24912
0.2,300.000us,0.70024,0.32936
0.2,400.000us,0.79184,0.41768
0.2,500.000us,0.86488,0.49128"""

# Read data into DataFrame
df = pd.read_csv(StringIO(data))

# Strip whitespace from column names
df.columns = df.columns.str.strip()

# Get unique deadlines
deadlines = df['Deadline'].unique()

# Create the plot
plt.figure(figsize=(10, 6))

markers = {
    '100.000us': 'X',
    '200.000us': 'D',
    '300.000us': '^',
    '400.000us': 'o',
    '500.000us': 'P'
}

# Plot DCT Acceptance Ratio vs Scale for each deadline
for deadline in deadlines:
    deadline_data = df[df['Deadline'] == deadline]
    plt.plot(deadline_data['Scale'], deadline_data['DCTAcceptanceRatio'], 
             marker=markers[deadline], markersize=12, linewidth=2, label=deadline)

plt.xlabel('Latency Scale', fontsize=32)
plt.ylabel('Acceptance Ratio', fontsize=32)
#plt.title('DCT Acceptance Ratio vs Scale')
plt.xticks(fontsize=32)
plt.yticks(fontsize=32)
plt.legend(title='Deadline', loc=(0, 1.05),fontsize=20, ncol=3, title_fontproperties={'size': 24, 'weight': 'bold'})
plt.grid(True, alpha=0.3)
plt.gca().invert_xaxis()
plt.tight_layout()
plt.savefig("dct_acceptance_vs_scale.pdf", dpi=300, bbox_inches="tight")
plt.close()