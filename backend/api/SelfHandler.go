package api

import (
	"backend/pkg/types"
	"net/http"
)

func SelfHandler(appState *types.AppContext) func(http.ResponseWriter, *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		token := r.Header.Get("Authorization")
		if token == "" {
			http.Error(w, "Missing authorization", http.StatusUnauthorized)
			return
		}

		user := types.UserFromToken(token, appState.GetDB().GetUserCollection())
		if user == nil {
			http.Error(w, "Unauthorized", http.StatusUnauthorized)
			return
		}

		b := user.ToAPI()
		if b == nil {
			http.Error(w, "Internal Server Error", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		_, _ = w.Write(b)
	}
}
