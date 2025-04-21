import subprocess
import re
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator
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
        'fn': float(re.search(r'False Negatives: (\d+\.\d+)', output).group(1)),
        'stream_time': float(re.search(r'Time to stream items into sketch: (\d+\.\d+)', output).group(1)),
        'count_time': float(re.search(r'Time to count \d+ items: (\d+\.\d+)', output).group(1))
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
        if sketch_type == 'cms':
            copt = f"-DNUM_BUCKETS={buckets}"
        elif sketch_type == 'cs':
            copt = f"-DCS_NUM_BUCKETS={buckets}"
        elif sketch_type == 'mg':
            copt = f"-DMG_MULT_FACTOR={buckets}"
        else:
            print(f"Unknown sketch type {sketch_type}")
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

    plt.xlabel(x_metric.replace('_', ' ').title())
    plt.ylabel('Percentage (%)')
    plt.title(title)
    plt.legend()
    plt.grid(True)

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    plt.savefig(f"{filename}_{timestamp}.png")
    plt.close()

def plot_time_analysis(data, count_time):
    """Plot streaming time vs sketch size with count time baseline"""
    plt.figure(figsize=(12, 6))

    # Plot count time baseline
    plt.axhline(y=count_time, color='r', linestyle='--',
                label='Baseline Count Time')

    # Plot streaming times
    for sketch in ['cms', 'cs', 'mg']:
        sketch_data = [d for d in data if d['sketch'] == sketch]
        if not sketch_data:
            continue

        sizes = [d['sketch_size'] for d in sketch_data]
        times = [d['stream_time'] for d in sketch_data]
        plt.plot(sizes, times, marker='o', linestyle='-',
                 color=COLORS[sketch], label=f'{sketch.upper()} Stream Time')

    plt.xlabel('Sketch Size (Bytes)')
    plt.ylabel('Time (seconds)')
    plt.title('Streaming Time vs Sketch Size')
    plt.legend()
    plt.grid(True)
    # plt.yscale('log')

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    plt.savefig(f"time_analysis_{timestamp}.png")
    plt.close()

def plot_additional_analysis(data):
    """Generate four additional analysis plots with fixed time formatting"""
    if not data:
        return

    plots_config = [
        {'x_metric': 'stream_time', 'y_metric': 'precision',
         'x_label': 'Update Time (seconds)', 'y_label': 'Precision/Recall (%)',
         'title': 'Precision and Recall vs Update Time'},
    ]

    for config in plots_config:
        plt.figure(figsize=(10, 6))

        # Group data by sketch type
        sketches = {}
        for d in data:
            sketch = d['sketch']
            if sketch not in sketches:
                sketches[sketch] = []
            sketches[sketch].append(d)

        # Plot each algorithm
        for sketch, points in sketches.items():
            # Sort points by x_metric
            sorted_points = sorted(points, key=lambda x: x[config['x_metric']])
            x_vals = [p[config['x_metric']] for p in sorted_points]
            y_vals = [p[config['y_metric']] for p in sorted_points]

            plt.plot(x_vals, y_vals,
                     marker='o', linestyle='-',
                     color=COLORS[sketch],
                     label=f'{sketch.upper()}')

        plt.xlabel(config['x_label'])
        plt.ylabel(config['y_label'])
        plt.title(config['title'])
        plt.legend()
        plt.grid(True)

        # Configure axis formatting
        if config['x_metric'] == 'stream_time':
            # Set x-axis formatting for seconds
            ax = plt.gca()
            max_time = max([p['stream_time'] for p in data])

            # Set appropriate tick spacing based on maximum time
            if max_time < 1:
                ax.xaxis.set_major_locator(MultipleLocator(0.1))
                ax.xaxis.set_minor_locator(MultipleLocator(0.02))
            elif max_time < 5:
                ax.xaxis.set_major_locator(MultipleLocator(0.5))
                ax.xaxis.set_minor_locator(MultipleLocator(0.1))
            else:
                ax.xaxis.set_major_locator(MultipleLocator(1))

            ax.xaxis.set_major_formatter(plt.FormatStrFormatter('%.2f'))
        else:
            # Format sketch size axis
            ax = plt.gca()
            ax.xaxis.set_major_formatter(
                plt.FuncFormatter(lambda x, _: f'{x/1000:.0f}KB' if x >= 1000 else f'{x}B'))

        plt.tight_layout()
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        plt.savefig(f"{config['title'].replace(' ', '_').lower()}_{timestamp}.png")
        plt.show()
        plt.close()

def main():
    phi_results = []
    for st in ['mg', 'cms', 'cs']:
        phi_results.extend(run_phi_experiment(st, DEFAULT_PHIS))

    print(phi_results)
    plot_metrics(phi_results, 'phi',
                [('precision', 'Precision'), ('recall', 'Recall')],
                'Precision/Recall vs Phi',
                'phi_sensitivity')

    memory_results = []

    memory_results.extend(run_memory_test('cms', MEM_TEST_BUCKETS))

    memory_results.extend(run_memory_test('cs', MEM_TEST_BUCKETS))

    memory_results.extend(run_memory_test('mg', [50, 100, 200, 400]))

    plot_metrics(memory_results, 'sketch_size',
                [('precision', 'Precision'), ('recall', 'Recall')],
                'Precision/Recall vs Sketch Size in Bytes',
                'memory_analysis')

    print(memory_results)

    if memory_results:
        count_time = memory_results[0]['count_time']  # Same across all runs
        plot_time_analysis(memory_results, count_time)
    plot_additional_analysis(memory_results)

if __name__ == "__main__":
    main()
