package server

import (
	"net/http"
	"strings"

	"github.com/gorilla/mux"

	"mymodule/internal/handlers"
	"mymodule/internal/httpx"
	"mymodule/internal/middleware"
	"mymodule/internal/paths"
)

func New() http.Handler {
	r := mux.NewRouter()

	r.Use(middleware.CORS)

	r.NotFoundHandler = http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		httpx.JSON(w, http.StatusNotFound, httpx.Error{Error: "not found"})
	})

	latestFS := http.FileServer(http.Dir(paths.FilesLatestDir()))
	r.PathPrefix("/files/latest/").Handler(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		sub := strings.TrimPrefix(r.URL.Path, "/files/latest/")
		if sub == "" || strings.HasSuffix(sub, "/") {
			httpx.JSON(w, http.StatusNotFound, httpx.Error{Error: "not found"})
			return
		}
		http.StripPrefix("/files/latest/", latestFS).ServeHTTP(w, r)
	})).Methods(http.MethodGet, http.MethodOptions)

	r.HandleFunc("/files/versioned/{hash}", handlers.VersionedFile()).
		Methods(http.MethodGet, http.MethodOptions)

	r.HandleFunc("/upload", handlers.Upload()).Methods(http.MethodPost, http.MethodOptions)

	return r
}
