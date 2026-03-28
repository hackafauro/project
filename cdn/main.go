package main

import (
	"log"
	"net/http"
	"os"

	"github.com/gorilla/handlers"

	"mymodule/internal/server"
)

func main() {
	port := os.Getenv("CDN_PORT")
	if port == "" {
		log.Fatal("CDN_PORT environment variable is not set")
	}

	h := server.New()

	log.Printf("listening on :%s", port)
	err := http.ListenAndServe(":"+port, handlers.LoggingHandler(os.Stdout, handlers.ProxyHeaders(h)))
	if err != nil {
		log.Fatal(err)
	}
}
