import subprocess
import re
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime
import os

# Configuration
PROGRAM_PATH = './test'
MAKE_CMD = 'make -B'
PHI_MEMORY_TEST = 0.001
N_MEMORY_TEST = 100_000_000
MEM_TEST_BUCKETS = [512, 1024, 2048, 4096, 8192]
DEFAULT_PHIS = [round(0.001 + i/1000, 3) for i in range(10)]
COLORS = {'cms': 'blue', 'cs': 'orange', 'mg': 'green'}

def run_command(cmd, cwd=None):
    """Run a shell command and return output"""
    print("Running command: ", cmd)
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd)
    if result.returncode != 0:
        print(f"Command failed: {' '.join(cmd)}")
        print(result.stderr)
        return None
    return result.stdout

def parse_output(output):
    """Parse program output to extract metrics"""
    metrics = {
        'precision': float(re.search(r'precision: (\d+\.\d+)', output).group(1)),
        'recall': float(re.search(r'recall: (\d+\.\d+)', output).group(1)),
        'sketch_size': int(re.search(r'Size of Sketch in Bytes: (\d+)', output).group(1)),
        'real_k': int(re.search(r'Real K value: (\d+)', output).group(1)),
        'tp': float(re.search(r'True Positives: (\d+\.\d+)', output).group(1)),
        'fp': float(re.search(r'False Positives: (\d+\.\d+)', output).group(1)),
        'fn': float(re.search(r'False Negatives: (\d+\.\d+)', output).group(1))
    }
    return metrics

def run_phi_experiment(sketch_type, phi_values, n=N_MEMORY_TEST):
    """Run phi sensitivity experiments"""
    results = []
    for phi in phi_values:
        cmd = [PROGRAM_PATH, str(n), str(phi), sketch_type]
        output = run_command(cmd)
        if output:
            metrics = parse_output(output)
            metrics.update({'phi': phi, 'sketch': sketch_type})
            results.append(metrics)
    return results

def run_memory_test(sketch_type, bucket_sizes):
    """Run memory vs accuracy tests with recompilation"""
    results = []
    for buckets in bucket_sizes:
        # Recompile with appropriate parameters
        if sketch_type == 'cms':
            copt = f"-DNUM_BUCKETS={buckets}"
        elif sketch_type == 'cs':
            copt = f"-DCS_NUM_BUCKETS={buckets}"
        elif sketch_type == 'mg':
            copt = f"-DMG_MULT_FACTOR={buckets}"
        else:
            printf(f"Unknown sketch type {sketch_type}")
            return results

        compile_cmd = f"{MAKE_CMD} COPT=\"{copt}\""
        run_command(compile_cmd.split(), cwd=os.path.dirname(PROGRAM_PATH))

        # Run test
        cmd = [PROGRAM_PATH, str(N_MEMORY_TEST), str(PHI_MEMORY_TEST), sketch_type]
        output = run_command(cmd)
        if output:
            metrics = parse_output(output)
            metrics.update({
                'buckets': buckets,
                'sketch': sketch_type,
                'phi': PHI_MEMORY_TEST
            })
            results.append(metrics)
    return results

def plot_metrics(data, x_metric, y_metrics, title, filename):
    """Generate and save plots with consistent styling"""
    plt.figure(figsize=(10, 6))

    # Group data by sketch type
    sketches = {d['sketch']: [] for d in data}
    for d in data:
        sketches[d['sketch']].append(d)

    for sketch, values in sketches.items():
        if not values:
            continue

        x = [v[x_metric] for v in values]
        for y_metric, style in zip(y_metrics, ['-', '--']):
            y = [v[y_metric[0]] for v in values]
            label = f"{sketch.upper()} {y_metric[1]}"
            plt.plot(x, y, style, marker='o',
                    color=COLORS[sketch], label=label)

    plt.xlabel("Sketch Size in bytes")
    plt.ylabel('Percentage (%)')
    plt.title(title)
    plt.legend()
    plt.grid(True)

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    plt.savefig(f"{filename}_{timestamp}.png")
    # plt.show()
    plt.close()

def main():
    # Run phi sensitivity tests
    phi_results = []
    for st in ['mg', 'cms', 'cs']:
        phi_results.extend(run_phi_experiment(st, DEFAULT_PHIS))

    # Plot phi sensitivity results
    plot_metrics(phi_results, 'phi',
                [('precision', 'Precision'), ('recall', 'Recall')],
                'Precision/Recall vs Phi',
                'phi_sensitivity')

    # Run memory vs accuracy tests
    memory_results = []

    # Test CMS with different buckets
    memory_results.extend(run_memory_test('cms', MEM_TEST_BUCKETS))

    # Test CS with different buckets
    memory_results.extend(run_memory_test('cs', MEM_TEST_BUCKETS))

    # Add MG baseline (no recompilation needed)
    memory_results.extend(run_memory_test('mg', [50, 100, 200, 400]))

    # Plot memory results
    plot_metrics(memory_results, 'sketch_size',
                [('precision', 'Precision'), ('recall', 'Recall')],
                'Precision/Recall vs Sketch Size in Bytes',
                'memory_analysis')

if __name__ == "__main__":
    main()
