import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import sys
import os
import matplotlib.patches as mpatches

# --- CONFIGURACIÓN ---
DEFAULT_GENERATION = 100 
POLICIES_ORDER = ["FIFO", "LTP", "STP", "RR_FIFO", "RR_LTP", "RR_ECA"]

# Las 30 semillas usadas en el C++
SEEDS_TO_PLOT = [0, 1, 2, 3, 5, 7, 11, 13, 17, 19, 
                 23, 29, 31, 37, 41, 43, 47, 53, 59, 61,
                 67, 71, 73, 79, 83, 89, 97, 101, 103, 107]

INSTANCES = ["Eg1", "Eg2", "Eg3"]

# --- FUNCIÓN PARA FILTRAR EL FRENTE GLOBAL ---
def get_global_front(df):
    """
    Recibe un DataFrame con columnas ['Time_Fitness', 'Energy_Fitness'].
    Retorna solo las filas que pertenecen al frente de Pareto GLOBAL.
    (Minimización de ambos objetivos)
    """
    if df.empty:
        return df
        
    # Convertimos a numpy para velocidad
    costs = df[['Time_Fitness', 'Energy_Fitness']].values
    is_efficient = np.ones(costs.shape[0], dtype=bool)
    
    for i, c in enumerate(costs):
        if is_efficient[i]:
            # Un punto c es dominado si existe otro punto 'other' tal que:
            # other <= c (en todos los objetivos) Y other < c (en al menos uno)
            # any(all(other <= c) and any(other < c))
            
            # Lógica vectorizada:
            # 1. Encontrar todos los que son menores o iguales en ambas dimensiones
            domination_check = np.all(costs <= c, axis=1)
            # 2. Asegurar que al menos uno sea estrictamente menor
            strict_check = np.any(costs < c, axis=1)
            
            # Combinar: El punto 'i' es dominado si existe algún 'j' que cumpla ambas
            is_dominated = np.any(domination_check & strict_check)
            
            if is_dominated:
                is_efficient[i] = False
                
    return df[is_efficient]

print("--- Generando Gráficas de Frentes de Pareto (Individual y Global) ---")

for instance in INSTANCES:
    print(f"\n================================")
    print(f" PROCESANDO INSTANCIA: {instance}")
    print(f"================================")

    INPUT_FILE = f"results/{instance}/all_checkpoint_fronts.csv"
    OUTPUT_FOLDER = f"plots/{instance}"
    if not os.path.exists(OUTPUT_FOLDER):
        os.makedirs(OUTPUT_FOLDER)

    try:
        df = pd.read_csv(INPUT_FILE)
    except Exception as e:
        print(f"  [ERROR] Al leer CSV o archivo no encontrado: {e}")
        continue

    # Filtrar datos base
    df_pareto = df[df['Rank'] == 1].copy()
    df_gen = df_pareto[df_pareto['Generation'] == DEFAULT_GENERATION]

    if df_gen.empty:
        continue

    # =========================================================================
    # CONFIGURACIÓN DE PLOTTEO
    # =========================================================================
    rows, cols = 6, 5
    # Definimos dos figuras: fig1 (Individual), fig2 (Global)
    modes = [("INDIVIDUAL", "Comparativa por Políticas"), ("GLOBAL", "Frente Global (Mejores Absolutos)")]

    for suffix, title_suffix in modes:
        print(f"  Generando gráfico modo: {suffix}...")
        
        fig, axes = plt.subplots(rows, cols, figsize=(20, 24), sharex=True, sharey=True)
        axes = axes.flatten()

        for i, seed in enumerate(SEEDS_TO_PLOT):
            if i >= len(axes): break
            ax = axes[i]
            
            # Datos de la semilla actual
            df_seed = df_gen[df_gen['Seed'] == seed].copy()
            
            if df_seed.empty:
                ax.text(0.5, 0.5, "Sin Datos", ha='center', va='center', color='red')
            else:
                # --- LOGICA DIFERENCIADA ---
                if suffix == "GLOBAL":
                    # Si es modo Global, recalculamos el frente usando TODAS las políticas juntas
                    df_to_plot = get_global_front(df_seed)
                else:
                    # Si es modo Individual, graficamos todo lo que llegó como Rank 1 de su propia política
                    df_to_plot = df_seed

                if df_to_plot.empty:
                    ax.text(0.5, 0.5, "Dominado Totalmente", ha='center', va='center', color='gray', fontsize=8)
                else:
                    sns.scatterplot(
                        data=df_to_plot,
                        x='Time_Fitness',
                        y='Energy_Fitness',
                        hue='Policy',
                        hue_order=POLICIES_ORDER,
                        palette='viridis',
                        s=60,
                        alpha=0.9,
                        ax=ax,
                        legend=False,
                        edgecolor='black', # Borde negro para que resalten más los puntos globales
                        linewidth=0.5
                    )

            ax.set_title(f'Seed {seed}', fontsize=10, fontweight='bold')
            ax.grid(True, linestyle=':', alpha=0.6)

        # Limpiar ejes sobrantes
        for j in range(len(SEEDS_TO_PLOT), len(axes)):
            axes[j].set_visible(False)

        # Decoración Global
        fig.suptitle(f'{instance} - {title_suffix} - Gen {DEFAULT_GENERATION}', fontsize=22, y=0.92)
        fig.text(0.5, 0.10, 'Tiempo (Makespan)', ha='center', va='center', fontsize=18)
        fig.text(0.08, 0.5, 'Energía Total', ha='center', va='center', rotation='vertical', fontsize=18)

        # Leyenda
        cmap = plt.colormaps.get_cmap('viridis')
        colors = cmap(np.linspace(0, 1, len(POLICIES_ORDER)))
        patches = [mpatches.Patch(color=colors[k], label=POLICIES_ORDER[k]) for k in range(len(POLICIES_ORDER))]
        
        fig.legend(handles=patches, bbox_to_anchor=(0.5, 0.05), loc='upper center', ncol=len(POLICIES_ORDER), title="Políticas", fontsize=12, title_fontsize=14)
        plt.subplots_adjust(top=0.88, bottom=0.15, left=0.12, right=0.95, hspace=0.3, wspace=0.1)

        # Guardar
        output_image = f"{OUTPUT_FOLDER}/pareto_{instance}_{suffix}.png"
        try:
            plt.savefig(output_image, dpi=150)
            plt.close(fig)
            # print(f"    -> Guardado: {output_image}")
        except Exception as e:
            print(f"    Error guardando imagen: {e}")

print("\n--- Proceso completado ---")