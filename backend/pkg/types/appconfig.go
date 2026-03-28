package types

import (
	"log"
	"os"
)

type AppConfig struct {
	listenAddr string
	dbUri      string
	apiKey     string
}

func NewAppConfig() *AppConfig {
	return &AppConfig{}
}

func (appConfig *AppConfig) LoadValidate() *AppConfig {
	appConfig.apiKey = os.Getenv("API_KEY")
	appConfig.dbUri = os.Getenv("DB_URI")
	appConfig.listenAddr = os.Getenv("APP_ADDR")

	if appConfig.apiKey == "" {
		log.Fatalln("GENAI_API_KEY is required")
	}

	if appConfig.dbUri == "" {
		log.Fatalln("DB_URI is required")
	}

	if appConfig.listenAddr == "" {
		log.Fatalln("APP_ADDR is required")
	}
	return appConfig
}

func (appConfig *AppConfig) GetDBUri() string {
	return appConfig.dbUri
}

func (appConfig *AppConfig) GetListenAddr() string {
	return appConfig.listenAddr
}

func (appConfig *AppConfig) GetAPIKey() string {
	return appConfig.apiKey
}
