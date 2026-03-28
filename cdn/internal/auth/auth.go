package auth

import (
	"net/http"
	"os"
	"strings"
)

func APIKeyFromRequest(r *http.Request) string {
	return strings.TrimSpace(r.Header.Get("Authorization"))
}

func Valid(key string) bool {
	if key == "" {
		return false
	}
	raw := os.Getenv("API_KEYS")
	for _, k := range strings.Split(raw, ";") {
		if strings.TrimSpace(k) == key {
			return true
		}
	}
	return false
}
