package api

import (
	"backend/pkg/types"
	"net/http"
)

func PurchaseHandler(appState *types.AppContext) func(http.ResponseWriter, *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		token := r.Header.Get("Authorization")
		if token == "" {
			http.Error(w, "Unauthorized", http.StatusUnauthorized)
			return
		}

		userCollection := appState.GetDB().GetUserCollection()
		purchasesCollection := appState.GetDB().GetPurchasesCollection()

		purchases := types.PurchasesFromToken(token, userCollection, purchasesCollection)
		if purchases == nil {
			http.Error(w, "Unauthorized", http.StatusUnauthorized)
			return
		}

		b := types.PurchasesToApi(purchases)
		if b == nil {
			http.Error(w, "Internal Server Error", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		_, _ = w.Write(b)
	}
}
