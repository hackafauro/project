package cmd

import (
	"backend/api"
	"backend/pkg/types"
	"net/http"
)

func Main() {
	appCtx := types.NewAppContext()
	defer appCtx.Close()

	appCtx.AddRoute("/api/self", api.SelfHandler(appCtx), http.MethodGet)
	appCtx.AddRoute("/api/search", api.SearchHandler(appCtx), http.MethodPost)
	appCtx.AddRoute("/api/details", api.DetailsHandler(appCtx), http.MethodPost)
	appCtx.AddRoute("/api/purchase", api.PurchaseHandler(appCtx), http.MethodPost)
	appCtx.AddRoute("/api/game", api.GameHandler(appCtx), http.MethodPost)

	appCtx.Start()
}
