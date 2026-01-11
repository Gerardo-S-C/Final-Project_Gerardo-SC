import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.patches as mpatches
import os

def plot_gantt_on_ax(ax, file_path, policy_name):
    try:
        df = pd.read_csv(file_path)
    except FileNotFoundError:
        ax.text(0.5, 0.5, f"Archivo no encontrado:\n{os.path.basename(file_path)}", 
                ha='center', va='center', color='red', fontsize=10)
        ax.set_title(f'Política: {policy_name} (Error)', color='red')
        return {} 
    except pd.errors.EmptyDataError:
        ax.text(0.5, 0.5, f"Archivo vacío:\n{os.path.basename(file_path)}", 
                ha='center', va='center', color='orange', fontsize=10)
        ax.set_title(f'Política: {policy_name} (Vacío)', color='orange')
        return {}

    jobs = sorted(df['Job_ID'].unique())
    if not jobs:
        ax.text(0.5, 0.5, f"Archivo sin datos:\n{os.path.basename(file_path)}", 
                ha='center', va='center', color='orange', fontsize=10)
        ax.set_title(f'Política: {policy_name} (Sin Datos)', color='orange')
        return {}
        
    cmap = plt.colormaps.get_cmap('viridis')
    colors = cmap(np.linspace(0, 1, len(jobs))) 
    job_colors = {job: colors[i] for i, job in enumerate(jobs)}

    for index, row in df.iterrows():
        start_time = row['Start_Time']
        end_time = row['End_Time']
        duration = end_time - start_time
        machine = row['Machine_ID']
        job = row['Job_ID']

        ax.barh(y=machine, width=duration, left=start_time,
                edgecolor='black', color=job_colors.get(job, 'gray'), alpha=0.8) 
        
        # Texto dentro de la barra
        ax.text(start_time + duration / 2, machine,
                f"J{job},O{row['Operation_ID']}",
                ha='center', va='center', color='white', fontsize=8, fontweight='bold')

    ax.set_ylabel('Máquina ID')
    ax.set_title(f'Política: {policy_name}')
    ax.set_yticks(sorted(df['Machine_ID'].unique()))
    ax.invert_yaxis()  
    ax.grid(axis='x', linestyle='--', alpha=0.6)

    patches = {f'Job {job}': mpatches.Patch(color=job_colors[job], label=f'Job {job}') for job in jobs}
    return patches

if __name__ == "__main__":

    policies = ["FIFO", "LTP", "STP", "RR_FIFO", "RR_LTP", "RR_ECA"]
    
    # 1. ACTUALIZAR LAS SEMILLAS (Las mismas 30 del C++)
    seeds = [0, 1, 2, 3, 5, 7, 11, 13, 17, 19, 
             23, 29, 31, 37, 41, 43, 47, 53, 59, 61,
             67, 71, 73, 79, 83, 89, 97, 101, 103, 107]
    
    # 2. AGREGAR LAS INSTANCIAS
    instances = ["Eg1", "Eg2", "Eg3"]

    for instance in instances:
        print(f"\n================================")
        print(f" PROCESANDO INSTANCIA: {instance}")
        print(f"================================")
        
        # Crear carpeta para guardar gráficas ordenadas
        output_folder = f"plots/{instance}"
        if not os.path.exists(output_folder):
            os.makedirs(output_folder)

        for seed in seeds:
            print(f"--- Procesando Semilla {seed} ---")
            
            # 3. ACTUALIZAR LA RUTA (Incluyendo la carpeta de instancia)
            # Nota: Asume que C++ guarda como seed_X_solution_1.txt
            file_paths = {
                policy: f"results/{instance}/{policy}/seed_{seed}_solution_1.txt" for policy in policies
            }

            fig, axes = plt.subplots(6, 1, figsize=(15, 22), sharex=True)

            all_patches = {}
            for i, policy in enumerate(policies):
                ax = axes[i] 
                file_path = file_paths[policy]
                
                # Pequeño log para debug
                # print(f"  Leyendo: {file_path}")
                
                patches = plot_gantt_on_ax(ax, file_path, policy)
                all_patches.update(patches)

            axes[-1].set_xlabel('Tiempo') 
            fig.suptitle(f'Diagramas de Gantt: {instance} - Semilla {seed}', fontsize=16, y=1.02)
            
            if all_patches:
                # Ordenar leyenda
                try:
                    sorted_labels = sorted(all_patches.keys(), key=lambda x: int(x.split(' ')[1]))
                    sorted_patches = [all_patches[label] for label in sorted_labels]
                    
                    fig.legend(handles=sorted_patches, 
                               bbox_to_anchor=(1.0, 0.95), 
                               loc='upper left', 
                               title="Trabajos (Jobs)")
                except:
                    # Fallback si falla el ordenamiento
                    fig.legend(handles=list(all_patches.values()), loc='upper right')
            else:
                print(f"  Advertencia: No se cargaron datos de Gantt para la Semilla {seed}.")

            plt.tight_layout(rect=[0, 0, 0.90, 1]) 
            
            # 4. GUARDAR EN LA CARPETA ORGANIZADA
            output_image = f'{output_folder}/gantt_{instance}_seed_{seed}.png'
            try:
                plt.savefig(output_image, bbox_inches='tight')
                # print(f"  Guardado: {output_image}")
            except Exception as e:
                print(f"  Error al guardar imagen: {e}")
            
            plt.close(fig) # Importante cerrar para liberar memoria con tantas graficas

    print("\n--- Proceso completado ---")