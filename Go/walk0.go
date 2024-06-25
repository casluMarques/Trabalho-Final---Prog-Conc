package main

import (
	"crypto/md5"
	"fmt"
	"io"
	"log"
	"os"
	"path/filepath"
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

	hash := md5.New()
	if _, err := io.Copy(hash, file); err != nil {
		log.Fatal(err)
	}

	return pair{fmt.Sprintf("%x", hash.Sum(nil)), path}
}

func searchTree(dir string) (results, time.Duration, time.Duration, error) {
	hashes := make(results)

	var filePaths []string

	//leitura
	startReading := time.Now()
	err := filepath.Walk(dir, func(p string, fi os.FileInfo, err error) error {
		if err != nil && err != os.ErrNotExist {
			return err
		}
		if fi.Mode().IsRegular() && fi.Size() > 0 {
			filePaths = append(filePaths, p)
		}
		return nil
	})
	readingTime := time.Since(startReading)

	if err != nil {
		return nil, 0, 0, err
	}

	// Cálculo dos hashes
	startHashing := time.Now()
	for _, path := range filePaths {
		h := hashFile(path)
		hashes[h.hash] = append(hashes[h.hash], h.path)
	}
	hashingTime := time.Since(startHashing)

	return hashes, readingTime, hashingTime, nil
}

// deleta as duplicatas
func deleteDuplicates(hashes results) time.Duration {
	start := time.Now()
	for _, files := range hashes {
		if len(files) > 1 {
			for i := 1; i < len(files); i++ {
				if err := os.Remove(files[i]); err != nil {
					log.Printf("Naõ conseguiu deletar: %s: %v", files[i], err)
				} else {
					fmt.Printf("Arquivo duplicado deletado: %s\n", files[i])
				}
			}
		}
	}
	return time.Since(start)
}

func main() {
	if len(os.Args) < 2 {
		log.Fatal("Passe o path para o diretório na linha de comando!")
	}

	start := time.Now()

	if hashes, readingTime, hashingTime, err := searchTree(os.Args[1]); err == nil {
		duplicationTime := deleteDuplicates(hashes)

		fmt.Printf("Tempo de leitura: %v\n", readingTime)
		fmt.Printf("Cálculo do Hash: %v\n", hashingTime)
		fmt.Printf("Tempo de exclusão: %v\n", duplicationTime)
	} else {
		log.Fatal(err)
	}

	fmt.Printf("Tempo total: %v\n", time.Since(start))
}
