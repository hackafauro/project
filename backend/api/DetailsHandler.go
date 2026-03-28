package api

import (
	ai2 "backend/pkg/ai"
	"backend/pkg/types"
	"encoding/json"
	"net/http"
)

func DetailsHandler(appState *types.AppContext) func(http.ResponseWriter, *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		type DetailsRequest struct {
			ProductId     string   `json:"productId"`
			TagsRequested []string `json:"tags"`
			SearchQuery   string   `json:"searchQuery"`
		}

		var req DetailsRequest
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			http.Error(w, "invalid request body", http.StatusBadRequest)
			return
		}

		product := types.GetProductByID(req.ProductId, appState.GetDB().GetItemsCollection())
		if product == nil {
			http.Error(w, "Product not found", http.StatusNotFound)
			return
		}

		ai, err := ai2.TransparencyFromPrompt(appState, r.Context(), "", req.SearchQuery, req.TagsRequested, product.Name, product.Description)
		if err != nil {
			http.Error(w, "AI processing error: "+err.Error(), http.StatusInternalServerError)
			return
		}

		product.AiWhy = ai

		b, err := json.Marshal(product)
		if err != nil {
			http.Error(w, "Internal Server Error", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		_, _ = w.Write(b)
	}
}
