package types

import (
	"backend/pkg/database"
	"errors"
	"log"
	"net/http"
	"os"
	"sync"
	"time"

	"github.com/gorilla/handlers"
	"github.com/gorilla/mux"
)

type AppContext struct {
	db     *database.Database
	mux    *mux.Router
	server http.Server
	config *AppConfig

	tagsMutex  sync.RWMutex
	tagsCache  []string
	tagsExpire time.Time
}

func NewAppContext() *AppContext {
	appCtx := &AppContext{}
	appCtx.config = NewAppConfig().LoadValidate()
	appCtx.db = database.NewDatabase(appCtx.config.GetDBUri())
	appCtx.mux = mux.NewRouter()
	return appCtx
}

func (appCtx *AppContext) GetDB() *database.Database {
	return appCtx.db
}

func (appCtx *AppContext) Start() {
	appCtx.mux.PathPrefix("/").Handler(http.FileServer(http.Dir("./web/")))

	addr := appCtx.config.GetListenAddr()
	appCtx.server = http.Server{
		ReadTimeout: 5 * time.Second,
		Addr:        addr,
		Handler:     handlers.LoggingHandler(os.Stdout, handlers.ProxyHeaders(handlers.RecoveryHandler()(appCtx.mux))),
	}

	log.Printf("Started server on http://%s\n", addr)
	if err := appCtx.server.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
		log.Fatalln(err)
	}
}

func (appCtx *AppContext) Close() {
	_ = appCtx.server.Close()
	appCtx.db.Close()
}

func (appCtx *AppContext) AddRoute(path string, handler http.HandlerFunc, methods ...string) {
	appCtx.mux.HandleFunc(path, handler).Methods(methods...)
}

func (appCtx *AppContext) GetTags() []string {
	appCtx.tagsMutex.RLock()
	defer appCtx.tagsMutex.RUnlock()

	if time.Now().Before(appCtx.tagsExpire) {
		return appCtx.tagsCache
	}

	return nil
}

func (appCtx *AppContext) SetTags(tags []string) {
	appCtx.tagsMutex.Lock()
	defer appCtx.tagsMutex.Unlock()

	appCtx.tagsExpire = time.Now().Add(15 * time.Minute)
	appCtx.tagsCache = tags
}

func (appCtx *AppContext) GetConfig() *AppConfig {
	return appCtx.config
}
