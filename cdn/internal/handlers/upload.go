package handlers

import (
	"crypto/sha256"
	"encoding/hex"
	"errors"
	"hash"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"mymodule/internal/auth"
	"mymodule/internal/httpx"
	"mymodule/internal/paths"
)

const maxUploadBytes = 1024 * 1024 * 1024

var errEmptyBody = errors.New("empty request body")

func Upload() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			httpx.JSON(w, http.StatusMethodNotAllowed, httpx.Error{Error: "method not allowed"})
			return
		}

		key := auth.APIKeyFromRequest(r)
		if key == "" {
			httpx.JSON(w, http.StatusBadRequest, httpx.Error{Error: "missing API key (Authorization header)"})
			return
		}
		if !auth.Valid(key) {
			httpx.JSON(w, http.StatusUnauthorized, httpx.Error{Error: "invalid API key"})
			return
		}

		versioned := versionedFromRequest(r)

		body := http.MaxBytesReader(w, r.Body, maxUploadBytes)
		defer body.Close()

		h := sha256.New()

		var dir string
		if versioned {
			dir = paths.FilesVersionedDir()
		} else {
			dir = paths.FilesLatestDir()
		}

		if err := writeContentAddressed(dir, body, h); err != nil {
			if errors.Is(err, errEmptyBody) {
				httpx.JSON(w, http.StatusBadRequest, httpx.Error{Error: "request body cannot be empty"})
				return
			}
			var maxErr *http.MaxBytesError
			if errors.As(err, &maxErr) {
				httpx.JSON(w, http.StatusRequestEntityTooLarge, httpx.Error{Error: "request body too large"})
				return
			}
			httpx.JSON(w, http.StatusInternalServerError, httpx.Error{Error: "storage failed"})
			return
		}

		hashHex := hex.EncodeToString(h.Sum(nil))
		path := downloadPath(versioned, hashHex)
		link := absoluteURL(r, path)
		if link == "" {
			httpx.JSON(w, http.StatusBadRequest, httpx.Error{Error: "missing Host header"})
			return
		}

		w.Header().Set("Content-Type", "text/plain; charset=utf-8")
		w.WriteHeader(http.StatusOK)
		_, _ = io.WriteString(w, link+"\n")
	}
}

func versionedFromRequest(r *http.Request) bool {
	if v := strings.TrimSpace(r.Header.Get("X-Versioned")); v != "" {
		b, err := strconv.ParseBool(v)
		if err == nil {
			return b
		}
	}
	q := strings.ToLower(strings.TrimSpace(r.URL.Query().Get("versioned")))
	switch q {
	case "1", "true", "yes", "on":
		return true
	case "0", "false", "no", "off", "":
		return false
	default:
		b, err := strconv.ParseBool(q)
		if err == nil {
			return b
		}
		return false
	}
}

func writeContentAddressed(dir string, body io.Reader, h hash.Hash) error {
	if err := os.MkdirAll(dir, 0o755); err != nil {
		return err
	}

	tmp, err := os.CreateTemp(dir, "upload-*")
	if err != nil {
		return err
	}
	tmpPath := tmp.Name()
	defer os.Remove(tmpPath)

	mw := io.MultiWriter(h, tmp)
	n, err := io.Copy(mw, body)
	if err != nil {
		tmp.Close()
		return err
	}
	if n == 0 {
		tmp.Close()
		return errEmptyBody
	}
	if err := tmp.Close(); err != nil {
		return err
	}

	hashHex := hex.EncodeToString(h.Sum(nil))
	final := filepath.Join(dir, hashHex)
	return os.Rename(tmpPath, final)
}

func downloadPath(versioned bool, hashHex string) string {
	if versioned {
		return "/files/versioned/" + hashHex
	}
	return "/files/latest/" + hashHex
}

func requestHost(r *http.Request) string {
	if h := strings.TrimSpace(r.Header.Get("X-Forwarded-Host")); h != "" {
		if i := strings.IndexByte(h, ','); i >= 0 {
			h = strings.TrimSpace(h[:i])
		}
		return h
	}
	return strings.TrimSpace(r.Host)
}

func requestScheme(r *http.Request) string {
	if p := strings.TrimSpace(r.Header.Get("X-Forwarded-Proto")); p != "" {
		if i := strings.IndexByte(p, ','); i >= 0 {
			p = strings.TrimSpace(p[:i])
		}
		return strings.ToLower(p)
	}
	if r.TLS != nil {
		return "https"
	}
	return "http"
}

func absoluteURL(r *http.Request, path string) string {
	host := requestHost(r)
	if host == "" {
		return ""
	}
	if !strings.HasPrefix(path, "/") {
		path = "/" + path
	}
	return requestScheme(r) + "://" + host + path
}
