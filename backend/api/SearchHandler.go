package api

import (
	"backend/pkg/ai"
	"backend/pkg/types"
	"context"
	"encoding/json"
	"net/http"
)

func SearchHandler(appState *types.AppContext) func(http.ResponseWriter, *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		var req map[string]string
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			http.Error(w, "invalid request body", http.StatusBadRequest)
			return
		}

		query, ok := req["query"]
		if !ok {
			http.Error(w, "`query` field is required", http.StatusBadRequest)
			return
		}

		dbTags := types.GetAllTags(appState, appState.GetDB().GetItemsCollection())
		if dbTags == nil {
			http.Error(w, "Internal Server Error", http.StatusInternalServerError)
			return
		}

		tags, err := ai.TagsFromPrompt(appState, context.Background(), "", dbTags, query)
		if err != nil {
			http.Error(w, "AI processing error: "+err.Error(), http.StatusInternalServerError)
			return
		}

		products := types.GetProductsByTag(tags, appState.GetDB().GetItemsCollection())
		if products == nil {
			http.Error(w, "Internal Server Error", http.StatusInternalServerError)
			return
		}

		type ProductResponse struct {
			Tags     []string          `json:"tags"`
			Products []types.DBProduct `json:"products"`
		}

		prodResponse := ProductResponse{
			Tags:     tags,
			Products: products,
		}

		b, err := json.Marshal(prodResponse)
		if err != nil {
			http.Error(w, "Internal Server Error", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		_, _ = w.Write(b)
	}
}
