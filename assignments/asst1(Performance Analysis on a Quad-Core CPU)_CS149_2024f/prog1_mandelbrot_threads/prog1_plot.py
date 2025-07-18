import subprocess
import re
import pandas as pd
import os
import numpy as np
import matplotlib.pyplot as plt

csv_filename = 'mandelbrot_results.csv'

time_ms = []
speedup_x = []

if not os.path.exists(csv_filename):
    # 批次运行 mandelbrot 程序
    for i in range(1, 33):
        # 测试
        # if i == 4: 
        #     break
        print(f'Running with {i} threads...')
        # 使用 subprocess 来调用命令行
        command_view_1 = ['./mandelbrot', '-t', str(i)]
        command_view_2 = ['./mandelbrot', '-t', str(i), '-v', '2']
        result = subprocess.run(command_view_1, capture_output=True, text=True, check=True)
        output = result.stdout
        # 正则匹配耗时（ms）和加速比（x）
        pattern_time_ms = r'.*\[(\d+\.?\d*)\] ms'
        pattern_speedup_x = r'(\d+\.?\d*)x speedup'

        match_time_ms = re.findall(pattern_time_ms, output)
        match_speedup_x = re.search(pattern_speedup_x, output)

        if match_time_ms:
            time_ms.append(float(match_time_ms[1]))
        else:
            raise ValueError(f'Time(ms) not found in output in {i} threads')
        if match_speedup_x:
            speedup_x.append(float(match_speedup_x.group(1)))
        else:
            raise ValueError(f'Speedup(x) not found in output in {i} threads')
        

    print('time_ms:', time_ms)
    print('speedup:', speedup_x)

    data_dict = {
        'Threads': list(range(1, 33)),
        'Time (ms)': time_ms,
        'Speedup (x)': speedup_x
    }

    df = pd.DataFrame(data_dict)
    df.to_csv(csv_filename, index=False, encoding='utf-8')

    print(f'原始数据保存到 {csv_filename} 中')
else:
    df = pd.read_csv(csv_filename)
    time_ms = df['Time (ms)'].tolist()
    speedup_x = df['Speedup (x)'].tolist()

plt.figure(figsize=(8, 6))
plt.plot(range(1, 33), speedup_x, marker='o', linestyle='-', color='b', label='Experiment Speedup')
plt.plot(range(1, 33), range(1, 33), linestyle='--', color='r', label='Ideal Speedup (y=x)')
plt.title('Mandelbrot Speedup vs Number of Threads')
plt.xlabel('Number of Threads')
plt.ylabel('Speedup (x)')
plt.xticks(range(1, 33))
plt.grid(True)
plt.legend()
plt.savefig('mandelbrot_speedup_plot.png')

plt.figure(figsize=(8, 6))
plt.plot(range(1, 33), time_ms, marker='o', linestyle='-', label='Time (ms)')
plt.title('Mandelbrot Time (ms) vs Number of Threads')
plt.xlabel('Number of Threads')
plt.ylabel('Time (ms)')
plt.xticks(range(1, 33))
plt.grid(True)
plt.legend()
plt.savefig('mandelbrot_time_ms_plot.png')

plt.show()