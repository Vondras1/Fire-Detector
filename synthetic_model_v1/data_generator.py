import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

# Set random seed 
np.random.seed(42)

# Number of samples
num_samples = 10000

# Inputs: 3 random numbers (each 0-1)
X = np.random.rand(num_samples, 3)

# Lables: 1 or 0
# (x1*0.2 + x2*0.5 + x3*0.3 > 0.7 → 1, else 0)
y = ((X[:,0]*0.2 + X[:,1]*0.5 + X[:,2]*0.3) > 0.7).astype(np.float32)

# Save generated data
df = pd.DataFrame(X, columns=["x1", "x2", "x3"])
df["labels"] = y
df.to_csv("data.csv", index=False)
