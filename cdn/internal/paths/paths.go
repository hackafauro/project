package paths

import (
	"log"
	"os"
	"path/filepath"
	"strings"
)

func RootDir() string {
	if v := strings.TrimSpace(os.Getenv("CDN_ROOT")); v != "" {
		return filepath.Clean(v)
	}
	wd, err := os.Getwd()
	if err != nil {
		log.Fatalf("CDN_ROOT not set and getwd failed: %v", err)
	}
	return wd
}

func FilesLatestDir() string {
	return filepath.Join(RootDir(), "files", "unversioned")
}

func FilesVersionedDir() string {
	return filepath.Join(RootDir(), "files", "versioned")
}
