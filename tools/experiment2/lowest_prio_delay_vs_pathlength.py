import pandas as pd
import matplotlib.pyplot as plt

# CSV data
data = """NumberofHops,DCT,RealFlow
2,200,200
4,690,780
6,1170,1520
8,1640,2400
10,2030,3120
12,2400,3440
14,2780,3960"""

# Read CSV data
from io import StringIO
#df = pd.read_csv('filename.csv')
df = pd.read_csv(StringIO(data))

# Create plot
plt.figure(figsize=(10, 6))
plt.plot(df['NumberofHops'], df['DCT'], marker='X', label='Edda', linewidth=2, markersize=12)
plt.plot(df['NumberofHops'], df['RealFlow'], marker='s', label='RealFlow', linewidth=2, markersize=12)

plt.xlabel('Number of Hops', fontsize=32)
plt.ylabel(r'Lowest Prio. Delay($\mu$s)', fontsize=32)
#plt.title('Delay vs Number of Hops (DCT vs RealFlow)', fontsize=14, fontweight='bold')
plt.legend(fontsize=28)
plt.xticks(fontsize=32)
plt.yticks(fontsize=32)
plt.grid(True, alpha=0.3)
plt.xticks(df['NumberofHops'])  # Show all hop values on x-axis
plt.tight_layout()

plt.savefig("lowest_priority_delay_vs_pathlength.pdf", dpi=300, bbox_inches="tight")
plt.close() 