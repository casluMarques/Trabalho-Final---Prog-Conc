import matplotlib.pyplot as plt

# Tempos em C
tempos_sequencial_c = [2.837870]
média_sequencial_c = sum(tempos_sequencial_c) / len(tempos_sequencial_c)

tempos_2_threads = [0.822753, 1.940000, 1.864063, 2.349486, 1.869500]
média_2_threads = sum(tempos_2_threads) / len(tempos_2_threads)

tempos_4_threads = [0.423763, 0.421536, 0.421805, 0.423673, 0.422310]
média_4_threads = sum(tempos_4_threads) / len(tempos_4_threads)

tempos_6_threads = [0.327063, 0.367369, 0.617368, 1.688296, 0.489562]
média_6_threads = sum(tempos_6_threads) / len(tempos_6_threads)

tempos_8_threads = [0.528304, 0.632444, 0.816366, 0.546796, 0.680389]
média_8_threads = sum(tempos_8_threads) / len(tempos_8_threads)

# Tempos em Go
média_sequencial_go = 1.747463

média_2_go = 1.060510
média_4_go = 0.645116
média_6_go = 0.505139
média_8_go = 0.445340

# Tempos em Python com threads
média_sequencial_py = 1.60

média_2_py = 1.32
média_4_py = 1.10
média_6_py = 1.08
média_8_py = 1.30

#Tempos em Python com Processos
média_sequencial_proc = média_sequencial_py

média_2_proc = 0.84
média_4_proc = 0.46
média_6_proc = 0.37
média_8_proc = 0.57

# Calculando aceleração
aceleração_c = [
    média_sequencial_c / média_2_threads,
    média_sequencial_c / média_4_threads,
    média_sequencial_c / média_6_threads,
    média_sequencial_c / média_8_threads
]

aceleração_go = [
    média_sequencial_go / média_2_go,
    média_sequencial_go / média_4_go,
    média_sequencial_go / média_6_go,
    média_sequencial_go / média_8_go
]

aceleração_py = [
    média_sequencial_py / média_2_py,
    média_sequencial_py / média_4_py,
    média_sequencial_py / média_6_py,
    média_sequencial_py / média_8_py
]

aceleração_proc = [
    média_sequencial_proc / média_2_proc,
    média_sequencial_proc / média_4_proc,
    média_sequencial_proc / média_6_proc,
    média_sequencial_proc / média_8_proc
]

# Plotando aceleração
threads = [2, 4, 6, 8]

plt.plot(threads, aceleração_c, marker='o', label='C')
plt.plot(threads, aceleração_go, marker='o', label='Go')
plt.plot(threads, aceleração_py, marker='o', label='Python - threads')
plt.plot(threads, aceleração_proc, marker='o', label='Python - processos')

plt.xlabel('Número de threads')
plt.ylabel('Aceleração')
plt.title('Aceleração vs Número de Threads')
plt.legend()
plt.grid(True)
plt.show()
