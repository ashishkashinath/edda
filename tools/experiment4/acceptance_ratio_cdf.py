import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Read the CSV file
# Replace 'acceptance_ratio_cdf.csv' with your actual filename
df = pd.read_csv('acceptance_ratio_cdf.csv')

# Strip whitespace from column names
df.columns = df.columns.str.strip()

d = np.sort(df['RealFlow'])
e = np.sort(df['DCT'])
c = np.arange(1, len(d)+1) / len(d)

plt.figure(figsize=(10, 6))
plt.plot(d, c, marker='o', color='blue', label='RealFlow', linewidth=2)
plt.plot(e, c, marker='o', color='orange', label='Edda', linewidth=2)

plt.xlabel('Acceptance Ratios', fontsize=28)         
plt.ylabel('CDF', fontsize=28)                  
#plt.title('CDF of Acceptance Ratios')       

plt.xticks(fontsize=28)
plt.yticks(fontsize=28)
plt.legend(loc='best', fontsize=24)
plt.grid()
plt.tight_layout()                       
plt.savefig("acceptance_ratio_cdf.pdf", dpi=300, bbox_inches="tight")
plt.close()

# Optional: Print some statistics
print("DCT Statistics:")
print(f"  Mean: {df['DCT'].mean():.4f}")
print(f"  Median: {df['DCT'].median():.4f}")
print(f"  Std Dev: {df['DCT'].std():.4f}")
print()
print("RealFlow Statistics:")
print(f"  Mean: {df['RealFlow'].mean():.4f}")
print(f"  Median: {df['RealFlow'].median():.4f}")
print(f"  Std Dev: {df['RealFlow'].std():.4f}")