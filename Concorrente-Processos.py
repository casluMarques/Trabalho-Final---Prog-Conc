import os
import hashlib
import time
import argparse
from collections import defaultdict
from multiprocessing import Pool

# Função para calcular o hash MD5 de um arquivo
def calculate_hash(file_path):
    try:
        with open(file_path, "rb") as f:
            md5 = hashlib.md5()
            while True:
                data = f.read(32768)
                if not data:
                    break
                md5.update(data)
            return file_path, md5.digest()
    except Exception as e:
        print(f"Erro ao calcular hash para {file_path}: {e}")
        return file_path, None

# Função para percorrer o diretório e obter a lista de todos os arquivos
def list_files(base_path):
    file_list = []
    for root, _, files in os.walk(base_path):
        for file in files:
            file_path = os.path.join(root, file)
            file_list.append(file_path)
    return file_list

# Função para deletar arquivos duplicados com base nos hashes
def delete_duplicates(hash_map):
    hash_to_paths = defaultdict(list)
    for file_path, hash_value in hash_map.items():
        hash_to_paths[hash_value].append(file_path)

    for hash_value, paths in hash_to_paths.items():
        if len(paths) > 1:
            for path in paths[1:]:
                try:
                    os.remove(path)
                    print(f"Arquivo duplicado deletado: {path}")
                except Exception as e:
                    print(f"Erro ao deletar {path}: {e}")

def main(directory_path, num_processos):
    # Medir o tempo total de execução
    start_time = time.time()

    # Medir o tempo para obter a lista de arquivos
    start_list_files = time.time()
    file_list = list_files(directory_path)
    end_list_files = time.time()

    # Medir o tempo para calcular os hashes dos arquivos
    start_hashing = time.time()
    hash_map = {}

    with Pool(processes=num_processos) as pool:
        results = pool.map(calculate_hash, file_list)
        for file_path, hash_value in results:
            if hash_value:
                hash_map[file_path] = hash_value

    end_hashing = time.time()

    # Exibindo resultados
    for file_path, hash_value in hash_map.items():
        print(f"Arquivo: {file_path}\nHash: {hash_value.hex()}")

    # Medir o tempo para deletar arquivos duplicados
    start_deletion = time.time()
    delete_duplicates(hash_map)
    end_deletion = time.time()

    # Tempo total de execução
    end_time = time.time()

    print(f"Tempo para listar arquivos: {end_list_files - start_list_files:.2f} segundos")
    print(f"Tempo para calcular hashes: {end_hashing - start_hashing:.2f} segundos")
    print(f"Tempo para deletar duplicatas: {end_deletion - start_deletion:.2f} segundos")
    print(f"Tempo total de execução: {end_time - start_time:.2f} segundos")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Deletar arquivos duplicados em um diretório")
    parser.add_argument("directory_path", type=str, help="Caminho do diretório para verificar duplicatas")
    parser.add_argument("num_processos", type=int, help="Número de processos a serem utilizados")
    args = parser.parse_args()

    main(args.directory_path, args.num_processos)
