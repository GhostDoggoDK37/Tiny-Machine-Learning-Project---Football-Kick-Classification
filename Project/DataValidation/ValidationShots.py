import numpy as np
import pandas as pd
import os

data_folder = r"c:\TML_Particle_EH5\Project\Project\DataValidation\Skud"
output_folder = r"c:\TML_Particle_EH5\Project\Project\DataValidation\Clipped_shots"
trigger_threshold = 7
window_after_trigger = 0.8   # sekunder efter trigger
pre_trigger_time = 0.2       # sekunder før trigger (100 ms)

os.makedirs(output_folder, exist_ok=True)

for filename in os.listdir(data_folder):
    if not filename.endswith(".csv"):
        continue

    path = os.path.join(data_folder, filename)
    print(f"\nBehandler {filename}...")

    # Prøv både semikolon og komma som separator
    try:
        data = pd.read_csv(path, sep=';', encoding='utf-8-sig')
        if data.shape[1] == 1:
            data = pd.read_csv(path, sep=',', encoding='utf-8-sig')
    except Exception as e:
        print(f"{filename}: Fejl ved læsning: {e}")
        continue

    # Fjern mellemrum og skjulte tegn fra kolonnenavne
    data.columns = [col.strip().replace('\ufeff', '') for col in data.columns]
    print("  Kolonner fundet:", list(data.columns))

    # Tjek at de nødvendige kolonner findes
    required_cols = {'timestamp', 'accX', 'accY', 'accZ'}
    if not required_cols.issubset(data.columns):
        print(f"{filename}: Forkert kolonneformat – springer over.")
        continue

    # --- KONVERTER DATA KORREKT ---
    def to_float(series):
        return pd.to_numeric(
            series.astype(str)
            .str.replace(",", ".", regex=False)
            .str.replace(" ", "", regex=False),
            errors="coerce"
        )

    data['timestamp'] = to_float(data['timestamp'])
    data['accX'] = to_float(data['accX'])
    data['accY'] = to_float(data['accY'])
    data['accZ'] = to_float(data['accZ'])

    data.dropna(subset=['timestamp', 'accX', 'accY', 'accZ'], inplace=True)

    # Konverter tid fra mikrosekunder til sekunder
    time = data['timestamp'].values * 1e-6
    ax = data['accX'].values
    ay = data['accY'].values
    az = data['accZ'].values

    # Beregn samlet acceleration (norm)
    a_total = np.sqrt(ax**2 + ay**2 + az**2)

    # Find trigger
    trigger_indices = np.where(a_total > trigger_threshold)[0]
    if len(trigger_indices) == 0:
        print(f"{filename}: Ingen trigger over {trigger_threshold}g – måling kasseres.")
        continue

    trigger_index = trigger_indices[0]
    trigger_time = time[trigger_index]

    # Beregn samplingsfrekvens
    dt = np.median(np.diff(time))
    fs = 1.0 / dt
    print(f"  Samplingsfrekvens estimeret til {fs:.1f} Hz")

    # --- KLIP VINDUE FØR OG EFTER TRIGGER ---
    samples_before_trigger = int(pre_trigger_time * fs)
    samples_after_trigger = int(window_after_trigger * fs)

    start_index = max(trigger_index - samples_before_trigger, 0)
    end_index = min(trigger_index + samples_after_trigger, len(time))

    clipped_indices = np.arange(start_index, end_index)

    # Justér tid så trigger = 0 s
    clipped_time = time[clipped_indices] - trigger_time
    clipped_ax = ax[clipped_indices]
    clipped_ay = ay[clipped_indices]
    clipped_az = az[clipped_indices]
    clipped_a_total = a_total[clipped_indices]

    # Gem klippet data
    output_name = filename.replace(".csv", "_clipped.csv")
    output_path = os.path.join(output_folder, output_name)
    pd.DataFrame({
        "time": clipped_time,
        "ax": clipped_ax,
        "ay": clipped_ay,
        "az": clipped_az,
        "a_total": clipped_a_total
    }).to_csv(output_path, index=False)
    print(f"  Klippet måling gemt som {output_name}")

print("\nFærdig. Alle valide målinger er klippet og gemt i 'Clipped_shots' mappen.")
