import pandas as pd
import matplotlib.pyplot as plt

# CSV data
data = """Deadline,DCT,RealFlow
0.000s,0,0
100.000us,0.47768,0.20688
200.000us,0.68168,0.3616
300.000us,0.82056,0.45856
400.000us,0.8868,0.54096
500.000us,0.93784,0.59368
600.000us,0.9736,0.64872
700.000us,0.9904,0.69072
800.000us,0.99824,0.72992
900.000us,0.99992,0.76712
1.000ms,1,0.80016
1.100ms,1,0.8312
1.200ms,1,0.86144
1.300ms,1,0.88896
1.400ms,1,0.90816
1.500ms,1,0.924
1.600ms,1,0.93776
1.700ms,1,0.9512
1.800ms,1,0.95968
1.900ms,1,0.97096
2.000ms,1,0.97968
2.100ms,1,0.9856
2.200ms,1,0.99056
2.300ms,1,0.99208
2.400ms,1,0.99528
2.500ms,1,0.99808
2.600ms,1,0.99856
2.700ms,1,0.9996
2.800ms,1,0.99992
2.900ms,1,1
3.000ms,1,1"""

# Read CSV data
from io import StringIO
df = pd.read_csv(StringIO(data))

# Convert deadline to numeric (microseconds)
def parse_deadline(deadline_str):
    if deadline_str.endswith('us'):
        return float(deadline_str[:-2])
    elif deadline_str.endswith('ms'):
        return float(deadline_str[:-2]) * 1000
    elif deadline_str.endswith('s'):
        return float(deadline_str[:-1]) * 1000000
    return float(deadline_str)

df['Deadline_us'] = df['Deadline'].apply(parse_deadline)

# Create plot
plt.figure(figsize=(10, 6))
plt.plot(df['Deadline_us'], df['DCT'], marker='X', markersize=12, label='Edda', linewidth=2)
plt.plot(df['Deadline_us'], df['RealFlow'], marker='s',markersize=12, label='RealFlow', linewidth=2)

plt.xlabel('Deadline (μs)', fontsize=32)
plt.ylabel('Acceptance Ratio', fontsize=32)
#plt.title('DCT vs RealFlow over Deadline', fontsize=14, fontweight='bold')
plt.xticks(fontsize=32)
plt.yticks(fontsize=32)
plt.legend(fontsize=28)
plt.grid(True, alpha=0.3)
plt.tight_layout()

plt.savefig("acceptance_ratio_vs_deadline_5switch_9links.pdf", dpi=300, bbox_inches="tight")
plt.close()