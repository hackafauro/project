package handlers

import (
	"fmt"
	"net/http"
	"os"
	"path/filepath"
	"regexp"

	"github.com/gorilla/mux"

	"mymodule/internal/httpx"
	"mymodule/internal/paths"
)

var sha256Hex = regexp.MustCompile(`^[a-f0-9]{64}$`)

func VersionedFile() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			httpx.JSON(w, http.StatusMethodNotAllowed, httpx.Error{Error: "method not allowed"})
			return
		}

		hash := mux.Vars(r)["hash"]
		if !sha256Hex.MatchString(hash) {
			httpx.JSON(w, http.StatusBadRequest, httpx.Error{Error: "invalid hash"})
			return
		}

		full := filepath.Join(paths.FilesVersionedDir(), hash)
		b, err := os.ReadFile(full)
		if err != nil {
			if os.IsNotExist(err) {
				httpx.JSON(w, http.StatusNotFound, httpx.Error{Error: "not found"})
				return
			}
			httpx.JSON(w, http.StatusInternalServerError, httpx.Error{Error: "read failed"})
			return
		}

		w.Header().Set("Content-Type", "application/octet-stream")
		w.Header().Set("Content-Disposition", fmt.Sprintf("attachment; filename=%q", hash))
		w.Header().Set("Content-Length", fmt.Sprint(len(b)))
		w.Header().Set("Content-Transfer-Encoding", "binary")
		w.WriteHeader(http.StatusOK)
		_, _ = w.Write(b)
	}
}
