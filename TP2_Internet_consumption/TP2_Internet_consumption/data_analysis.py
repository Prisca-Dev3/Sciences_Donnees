import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os
import matplotlib.dates as mdates

# ==============================================================================
# Complete Data Analysis of the Processed Internet Consumption Dataset
# ==============================================================================

# Setting larger default sizes for figures
plt.rcParams['figure.figsize'] = (12, 6)
plt.rcParams['font.size'] = 12
plt.rcParams['axes.titlesize'] = 16

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROCESSED_DIR = os.path.join(BASE_DIR, "datasets", "data", "processed")
OUTPUT_DIR = os.path.join(BASE_DIR, "analysis_plots")

# Create output directory if it doesn't exist
if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

print("Starting Data Analysis...")
print(f"Plots will be saved in: {OUTPUT_DIR}\n")

# Load processed data
file_path = os.path.join(PROCESSED_DIR, "internet_consumption_complete.csv")
print(f"Loading data from {file_path}...")
df = pd.read_csv(file_path)

# Convert datetime to proper pandas datetime object
df["datetime"] = pd.to_datetime(df["datetime"])

# Summary of the dataset
print("\nDataset Summary:")
print(df.info())
print("\nDataset Description:")
print(df.describe())

# ------------------------------------------------------------------------------
# 1. Consumption Over Time (Time Series aggregations)
# ------------------------------------------------------------------------------
print("1. Generating Time Series Plots for multiple aggregations...")
df_time = df.set_index("datetime")

# Multiple aggregations to track
aggregations = [
    ("15min", "15 Minutes"),
    ("30min", "30 Minutes"),
    ("45min", "45 Minutes"),
    ("1h", "1 Hour") # Corrected from "H" to lowercase "h" if we were doing single
]

fig, axes = plt.subplots(2, 2, figsize=(16, 10))
axes = axes.flatten()

for i, (freq, label) in enumerate(aggregations):
    ax = axes[i]
    agg_data = df_time["total_Mo"].resample(freq).sum()
    
    ax.plot(agg_data.index, agg_data.values, color='#1f77b4', linewidth=1.5)
    ax.set_title(f"Aggregation: {label}")
    ax.set_xlabel("Date and Time")
    ax.set_ylabel("Total Consumption (Mo)")
    ax.grid(True, alpha=0.3)
    ax.tick_params(axis='x', rotation=45)

plt.suptitle("Total Internet Consumption Over Time (Different Time Aggregations)", fontsize=18)
plt.tight_layout()
plt.savefig(os.path.join(OUTPUT_DIR, "1_consumption_time_series.png"), dpi=300)
plt.close()

# ------------------------------------------------------------------------------
# 2. Consumption by Day of the Week
# ------------------------------------------------------------------------------
print("2. Generating Consumption by Day of Week Plot...")
# Define the order of days
days_order = ['Lundi', 'Mardi', 'Mercredi', 'Jeudi', 'Vendredi', 'Samedi', 'Dimanche']
df['jour_semaine'] = pd.Categorical(df['jour_semaine'], categories=days_order, ordered=True)

# Calculate sum and mean
day_sum = df.groupby('jour_semaine', observed=False)['total_Mo'].sum()

plt.figure()
bars = plt.bar(day_sum.index, day_sum.values, color='#ff7f0e', alpha=0.8)
plt.title("Total Data Consumption by Day of the Week")
plt.xlabel("Day of the Week")
plt.ylabel("Total Consumption (Mo)")
plt.grid(axis='y', alpha=0.3)

# Add values on top of bars
for bar in bars:
    yval = bar.get_height()
    plt.text(bar.get_x() + bar.get_width()/2, yval + (yval*0.01), f'{yval:.1f}', ha='center', va='bottom', fontsize=10)

plt.tight_layout()
plt.savefig(os.path.join(OUTPUT_DIR, "2_consumption_by_day.png"), dpi=300)
plt.close()

# ------------------------------------------------------------------------------
# 3. Consumption by Period of the Day
# ------------------------------------------------------------------------------
print("3. Generating Consumption by Period Plot...")
# Calculate sum
period_sum = df.groupby('periode_journee', observed=False)['total_Mo'].sum().sort_values(ascending=False)

plt.figure(figsize=(12, 7))
bars = plt.bar(period_sum.index, period_sum.values, color='#2ca02c', alpha=0.8)
plt.title("Total Data Consumption by Period of the Day")
plt.xlabel("Period of the Day")
plt.ylabel("Total Consumption (Mo)")
plt.xticks(rotation=45, ha='right')
plt.grid(axis='y', alpha=0.3)

for bar in bars:
    yval = bar.get_height()
    plt.text(bar.get_x() + bar.get_width()/2, yval + (yval*0.01), f'{yval:.1f}', ha='center', va='bottom', fontsize=10)

plt.tight_layout()
plt.savefig(os.path.join(OUTPUT_DIR, "3_consumption_by_period.png"), dpi=300)
plt.close()

# ------------------------------------------------------------------------------
# 4. Download vs Upload Distribution (Boxplots & Histograms)
# ------------------------------------------------------------------------------
print("4. Generating Distribution Plots for Download and Upload...")
# Only plot data < 99th percentile to avoid extreme outliers squashing the plot
q_dl = df['download_Mo'].quantile(0.99)
q_ul = df['upload_Mo'].quantile(0.99)

filtered_df = df[(df['download_Mo'] < q_dl) & (df['upload_Mo'] < q_ul)]

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

ax1.hist(filtered_df['download_Mo'], bins=50, color='#9467bd', alpha=0.7)
ax1.set_title(f'Download Distribution (< 99th Percentile: {q_dl:.2f} Mo)')
ax1.set_xlabel('Download (Mo)')
ax1.set_ylabel('Frequency')
ax1.grid(alpha=0.3)

ax2.hist(filtered_df['upload_Mo'], bins=50, color='#8c564b', alpha=0.7)
ax2.set_title(f'Upload Distribution (< 99th Percentile: {q_ul:.2f} Mo)')
ax2.set_xlabel('Upload (Mo)')
ax2.grid(alpha=0.3)

plt.tight_layout()
plt.savefig(os.path.join(OUTPUT_DIR, "4_download_upload_distribution.png"), dpi=300)
plt.close()

# Boxplot comparing Download and Upload directly
plt.figure()
plt.boxplot([filtered_df['download_Mo'], filtered_df['upload_Mo']], tick_labels=['Download (Mo)', 'Upload (Mo)'])
plt.title("Boxplot of Download vs Upload (Outliers excluded)")
plt.ylabel("Volume (Mo)")
plt.grid(axis='y', alpha=0.3)
plt.tight_layout()
plt.savefig(os.path.join(OUTPUT_DIR, "5_boxplot_dl_vs_up.png"), dpi=300)
plt.close()

# ------------------------------------------------------------------------------
# 5. Proportion of Connection Status
# ------------------------------------------------------------------------------
print("5. Generating Connection Status Pie Chart...")
status_counts = df['statut_connexion'].value_counts()

plt.figure(figsize=(8, 8))
plt.pie(status_counts, labels=status_counts.index, autopct='%1.1f%%', startangle=90, colors=['#d62728', '#7f7f7f'], shadow=True)
plt.title("Connection Status Distribution")
plt.tight_layout()
plt.savefig(os.path.join(OUTPUT_DIR, "6_connection_status.png"), dpi=300)
plt.close()

# ------------------------------------------------------------------------------
# 6. Scatter Plot Download vs Upload
# ------------------------------------------------------------------------------
print("6. Generating Download vs Upload Scatter Plot...")
plt.figure(figsize=(10, 8))
plt.scatter(filtered_df['download_Mo'], filtered_df['upload_Mo'], alpha=0.3, s=10, color='#17becf')
plt.title("Scatter Plot: Download vs Upload Volume")
plt.xlabel("Download (Mo)")
plt.ylabel("Upload (Mo)")
plt.grid(True, alpha=0.3)

# Add identity line
max_val = max(filtered_df['download_Mo'].max(), filtered_df['upload_Mo'].max())
plt.plot([0, max_val], [0, max_val], 'r--', alpha=0.5, label='1:1 Ratio')
plt.legend()

plt.tight_layout()
plt.savefig(os.path.join(OUTPUT_DIR, "7_scatter_dl_vs_up.png"), dpi=300)
plt.close()

# ------------------------------------------------------------------------------
# 7. Correlation Matrix
# ------------------------------------------------------------------------------
print("7. Generating Correlation Matrix...")
numeric_df = df[['download_Mo', 'upload_Mo', 'total_Mo', 'duree_minutes']]
corr_matrix = numeric_df.corr()

plt.figure(figsize=(8, 6))
cax = plt.matshow(corr_matrix, cmap='coolwarm', vmin=-1, vmax=1, fignum=1)
plt.colorbar(cax)

plt.xticks(range(len(corr_matrix.columns)), corr_matrix.columns, rotation=45, ha='left')
plt.yticks(range(len(corr_matrix.columns)), corr_matrix.columns)

for (i, j), val in np.ndenumerate(corr_matrix):
    plt.text(j, i, f'{val:.2f}', ha='center', va='center', 
             color='white' if abs(val) > 0.5 else 'black')

plt.title("Correlation Matrix of Numeric Variables", pad=20)

plt.savefig(os.path.join(OUTPUT_DIR, "8_correlation_matrix.png"), dpi=300)
plt.close()

print("\nAnalysis Complete! ✅")
print(f"You can view all the generated diagrams in the '{OUTPUT_DIR}' directory.")
