import numpy as np
import pandas as pd
import os

data_folder = r"c:\TML_Particle_EH5\Project\Project\DataValidation\Noise"
output_folder = r"c:\TML_Particle_EH5\Project\Project\DataValidation\Clipped_noise"
window_length_s = 1    # 1 sekund
segments_per_file = 1  # Kun ét segment

os.makedirs(output_folder, exist_ok=True)

def to_float(s):
    return pd.to_numeric(
        s.astype(str)
         .str.replace(",", ".", regex=False)
         .str.replace(" ", "", regex=False),
        errors="coerce"
    )

for filename in os.listdir(data_folder):
    if not filename.lower().endswith(".csv"):
        continue

    path = os.path.join(data_folder, filename)
    print(f"\nBehandler {filename}...")

    try:
        data = pd.read_csv(path, sep=';', encoding='utf-8-sig')
        if data.shape[1] == 1:
            data = pd.read_csv(path, sep=',', encoding='utf-8-sig')
    except Exception as e:
        print(f"{filename}: Fejl ved læsning: {e}")
        continue

    data.columns = [c.strip().replace('\ufeff', '') for c in data.columns]

    req = {"timestamp", "accX", "accY", "accZ"}
    if not req.issubset(data.columns):
        print("  Forkert format – springer over.")
        continue

    # Konverter
    data["timestamp"] = to_float(data["timestamp"])
    data["accX"] = to_float(data["accX"])
    data["accY"] = to_float(data["accY"])
    data["accZ"] = to_float(data["accZ"])
    data.dropna(subset=["timestamp", "accX", "accY", "accZ"], inplace=True)

    time = data["timestamp"].values * 1e-6
    ax = data["accX"].values
    ay = data["accY"].values
    az = data["accZ"].values

    if len(time) < 20:
        print("  For få samples – springer over.")
        continue

    dt = np.median(np.diff(time))
    fs = 1.0 / dt
    win_samples = int(window_length_s * fs)

    if len(time) < win_samples:
        print("  Filen er for kort – springer over.")
        continue

    # MIDT-SEGMENT:
    mid = len(time) // 2
    start = mid - win_samples // 2
    end = start + win_samples

    if start < 0:
        start = 0
        end = win_samples
    if end > len(time):
        end = len(time)
        start = end - win_samples

    seg_time = time[start:end] - time[start]
    seg_ax = ax[start:end]
    seg_ay = ay[start:end]
    seg_az = az[start:end]
    seg_atotal = np.sqrt(seg_ax**2 + seg_ay**2 + seg_az**2)

    out_name = filename.replace(".csv", ".csv")
    out_path = os.path.join(output_folder, out_name)

    pd.DataFrame({
        "time": seg_time,
        "ax": seg_ax,
        "ay": seg_ay,
        "az": seg_az,
        "a_total": seg_atotal
    }).to_csv(out_path, index=False)

    print("  Lavede midt-segment.")

print("\nFærdig.")
