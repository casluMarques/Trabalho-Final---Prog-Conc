package main

import (
	"crypto/md5"
	"fmt"
	"io"
	"log"
	"os"
	"path/filepath"
	"strconv"
	"time"
)

type pair struct {
	hash string
	path string
}

type fileList []string
type results map[string]fileList

func hashFile(path string) pair {
	file, err := os.Open(path)
	if err != nil {
		log.Fatal(err)
	}
	defer file.Close()

	hash := md5.New() // rápido e suficiente
	if _, err := io.Copy(hash, file); err != nil {
		log.Fatal(err)
	}

	return pair{fmt.Sprintf("%x", hash.Sum(nil)), path}
}

func processFiles(paths <-chan string, pairs chan<- pair, done chan<- bool) {
	for path := range paths {
		pairs <- hashFile(path)
	}
	done <- true
}

func collectHashes(pairs <-chan pair, result chan<- results) {
	hashes := make(results)
	for p := range pairs {
		hashes[p.hash] = append(hashes[p.hash], p.path)
	}
	result <- hashes
}

func searchTree(dir string, paths chan<- string) (time.Duration, error) {
	start := time.Now()
	visit := func(p string, fi os.FileInfo, err error) error {
		if err != nil && err != os.ErrNotExist {
			return err
		}
		if fi.Mode().IsRegular() && fi.Size() > 0 {
			paths <- p
		}
		return nil

	}
	readingTime := time.Since(start)
	err := filepath.Walk(dir, visit)
	return readingTime, err
}

func deleteDuplicates(hashes results) time.Duration {
	start := time.Now()
	for _, files := range hashes {
		if len(files) > 1 {
			for i := 1; i < len(files); i++ {
				if err := os.Remove(files[i]); err != nil {
					log.Printf("Falha ao deletar %s: %v", files[i], err)
				} else {
					fmt.Printf("Arquivo duplicado deletado: %s\n", files[i])
				}
			}
		}
	}
	return time.Since(start)
}

func run(dir string, workers int) (results, time.Duration, time.Duration, time.Duration) {
	paths := make(chan string)
	pairs := make(chan pair)
	done := make(chan bool)
	result := make(chan results)

	startHashing := time.Now()

	for i := 0; i < workers; i++ {
		go processFiles(paths, pairs, done)
	}

	go collectHashes(pairs, result)

	readingTime, err := searchTree(dir, paths)
	if err != nil {
		log.Fatal(err)
	}

	close(paths)

	for i := 0; i < workers; i++ {
		<-done
	}
	close(pairs)

	hashes := <-result
	hashingTime := time.Since(startHashing)
	return hashes, readingTime, hashingTime, 0
}

func main() {
	if len(os.Args) < 3 {
		log.Fatal("Faltando parâmetros. Uso: <programa> <diretório> <num_trabalhadores>")
	}

	dir := os.Args[1]
	workers, err := strconv.Atoi(os.Args[2])
	if err != nil || workers <= 0 {
		log.Fatal("Número de trabalhadores inválido")
	}

	start := time.Now()

	hashes, readingTime, hashingTime, _ := run(dir, workers)
	duplicationTime := deleteDuplicates(hashes)

	totalTime := time.Since(start)

	fmt.Printf("Número de trabalhadores: %d\n", workers)
	fmt.Printf("Tempo de leitura: %v\n", readingTime)
	fmt.Printf("Tempo de hash: %v\n", hashingTime)
	fmt.Printf("Tempo de exclusão: %v\n", duplicationTime)
	fmt.Printf("Tempo total de execução: %v\n", totalTime)
}
