package api

import (
	"backend/pkg/types"
	"encoding/json"
	"net/http"
)

func GameHandler(appState *types.AppContext) func(http.ResponseWriter, *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		type GameRequest struct {
			Win bool `json:"win"`
		}

		var req GameRequest
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			http.Error(w, "Invalid request body", http.StatusBadRequest)
			return
		}

		token := r.Header.Get("Authorization")
		if token == "" {
			http.Error(w, "Unauthorized", http.StatusUnauthorized)
			return
		}

		user := types.UserFromToken(token, appState.GetDB().GetUserCollection())
		if user == nil {
			http.Error(w, "Unauthorized", http.StatusUnauthorized)
			return
		}

		if !user.IsDiscountPlayable() {
			http.Error(w, "User not eligible to play", http.StatusForbidden)
			return
		}

		err := user.GameResult(req.Win, appState.GetDB().GetUserCollection())
		if err != nil {
			http.Error(w, "Internal Server Error", http.StatusInternalServerError)
			return
		}

		type GameResponse struct {
			Ok bool `json:"ok"`
		}

		resp := GameResponse{Ok: true}
		b, err := json.Marshal(resp)
		if err != nil {
			http.Error(w, "Internal Server Error", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		_, _ = w.Write(b)
	}
}
