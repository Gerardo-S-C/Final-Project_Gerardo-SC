import pandas as pd
from pymoo.indicators.hv import HV
import numpy as np
import warnings
import sys
import os

# Configuración General
# NOTA: Si las instancias Eg2 o Eg3 tienen valores de tiempo/energía mucho más grandes,
# tal vez necesites aumentar este punto de referencia (ej. [500.0, 500.0]).
ref_point = np.array([1000.0, 1000.0]) 

hv_calculator = HV(ref_point=ref_point, normalize=False)

policies_order = ["FIFO", "LTP", "STP", "RR_FIFO", "RR_LTP", "RR_ECA"]
stats_order = ['min', 'max', 'Prom (desv)']
instances = ["Eg1", "Eg2", "Eg3"]

def calculate_hv(group):
    # Seleccionamos solo las columnas de fitness
    points = group[['Time_Fitness', 'Energy_Fitness']].values
    if points.shape[0] == 0:
        return 0.0
    return hv_calculator.do(points)

print("--- Iniciando Cálculo de Hipervolumen por Instancia ---")

for instance in instances:
    print(f"\nProcesando instancia: {instance}...")
    
    # Rutas dinámicas por instancia
    input_file = f"results/{instance}/all_checkpoint_fronts.csv"
    output_file = f"results/{instance}/hypervolume_report.csv"

    if not os.path.exists(input_file):
        print(f"  [AVISO] No se encontró el archivo: {input_file}. Saltando...")
        continue

    try:
        df = pd.read_csv(input_file)
    except Exception as e:
        print(f"  [ERROR] Al leer CSV: {e}")
        continue

    # Filtrar solo el primer frente (Rank 1)
    df_pareto = df[df['Rank'] == 1].copy()

    if df_pareto.empty:
        print(f"  [AVISO] El CSV de {instance} no contiene soluciones con 'Rank == 1'.")
        continue

    # Calcular HV agrupando por Semilla, Generación y Política
    # Esto nos da un valor de HV por cada "punto" en el tiempo de cada ejecución
    hv_data = df_pareto.groupby(['Seed', 'Generation', 'Policy']).apply(calculate_hv)
    hv_data = hv_data.reset_index(name='Hypervolume')

    # Calcular estadísticas (Promedio y Desviación) sobre las 30 semillas
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", category=RuntimeWarning)
        final_stats = hv_data.groupby(['Generation', 'Policy'])['Hypervolume'].agg(
            min='min', 
            max='max', 
            prom='mean', 
            desv='std'
        ).reset_index()

    final_stats['desv'] = final_stats['desv'].fillna(0)

    # Formatear columna combinada "Promedio (Desviación)"
    final_stats['Prom (desv)'] = final_stats.apply(
        lambda row: f"{row['prom']:.2f} ({row['desv']:.2f})", 
        axis=1
    )
    
    # Limpiar columnas auxiliares
    final_stats = final_stats.drop(columns=['prom', 'desv'])

    # Crear tabla pivote para visualizar mejor
    pivot_table = final_stats.pivot(
        index='Generation', 
        columns='Policy', 
        values=['min', 'max', 'Prom (desv)']
    )

    # Reordenar columnas para que queden agrupadas por política
    try:
        # Generar todas las combinaciones posibles de columnas
        final_columns = [(stat, policy) for policy in policies_order for stat in stats_order]
        
        # Filtrar solo las que existen en la tabla (por si alguna política no generó datos)
        available_columns = pivot_table.columns
        valid_columns = [col for col in final_columns if col in available_columns]
        
        pivot_table = pivot_table.reindex(columns=valid_columns)
    except Exception as e:
        print(f"  Advertencia reordenando columnas: {e}")
        
    # Guardar reporte
    try:
        pivot_table.to_csv(output_file)
        print(f"  -> Reporte guardado exitosamente en: '{output_file}'")
    except Exception as e:
        print(f"  [ERROR] No se pudo guardar el archivo {output_file}: {e}")

print("\n--- Proceso completado ---")