import pandas as pd
import matplotlib.pyplot as plt

# Data
data = """Priority, Scale, NumberOfHops
Low, 1.0, 252
Low, 0.9, 254
Low, 0.8, 256
Low, 0.7, 262
Low, 0.6, 272
Low, 0.5, 286
Low, 0.4, 298
Low, 0.3, 320
Medium, 1.0, 268
Medium, 0.9, 276
Medium, 0.8, 292
Medium, 0.7, 310
Medium, 0.6, 336
Medium, 0.5, 366
Medium, 0.4, 394
Medium, 0.3, 424
High, 1.0, 808
High, 0.9, 823
High, 0.8, 848
High, 0.7, 868
High, 0.6, 898
High, 0.5, 922
High, 0.4, 945
High, 0.3, 955"""

# Read data into DataFrame
from io import StringIO
df = pd.read_csv(StringIO(data))

# Strip whitespace from column names
df.columns = df.columns.str.strip()

# Create the plot
plt.figure(figsize=(10, 6))

colors = {
    'High': '#e41a1c',
    'Medium': '#377eb8',
    'Low': '#4daf4a'
}

markers = {
    'High': 'X',
    'Medium': 'D',
    'Low': '^'
}

# Plot each priority level
for priority in ['Low', 'Medium', 'High']:
    priority_data = df[df['Priority'].str.strip() == priority]
    plt.plot(priority_data['Scale'], priority_data['NumberOfHops'], 
             marker=markers[priority], markersize=12, linewidth=2, color = colors[priority], label=f'{priority} Priority')

# Customize the plot
plt.xlabel('Latency Scale', fontsize=32)
plt.ylabel('Number of Hops', fontsize=32)
plt.xticks(fontsize=32)
plt.yticks(fontsize=32)
#plt.title('Number of Hops vs Scale for Different Priority Flows', fontsize=14, fontweight='bold')
plt.legend(fontsize=28)
plt.grid(True, alpha=0.3)
plt.xticks([0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0])

# Invert x-axis so scale goes from 1.0 to 0.3 (left to right)
plt.gca().invert_xaxis()

# plt.tight_layout()
# plt.show()

plt.savefig("adaptive_routing_hops_fix.pdf", dpi=300, bbox_inches="tight")
plt.close()