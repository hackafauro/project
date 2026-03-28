package database

import (
	"context"
	"log"

	"go.mongodb.org/mongo-driver/v2/mongo"
	"go.mongodb.org/mongo-driver/v2/mongo/options"
)

type Database struct {
	client *mongo.Client

	usersColl     *mongo.Collection
	itemsColl     *mongo.Collection
	purchasesColl *mongo.Collection
}

func connect(uri string) *mongo.Client {
	serverAPI := options.ServerAPI(options.ServerAPIVersion1)
	opts := options.Client().ApplyURI(uri).SetServerAPIOptions(serverAPI)
	client, err := mongo.Connect(opts)
	if err != nil {
		log.Fatalln(err)
	}

	return client
}

func (d *Database) LoadUsersCollections() {
	d.usersColl = d.client.Database("db").Collection("users")
	d.itemsColl = d.client.Database("db").Collection("items")
	d.purchasesColl = d.client.Database("db").Collection("purchases")
}

func NewDatabase(uri string) *Database {
	d := &Database{}
	d.client = connect(uri)
	d.LoadUsersCollections()

	return d
}

func (d *Database) Close() {
	_ = d.client.Disconnect(context.Background())
}

func (d *Database) GetUserCollection() *mongo.Collection {
	return d.usersColl
}

func (d *Database) GetItemsCollection() *mongo.Collection {
	return d.itemsColl
}

func (d *Database) GetPurchasesCollection() *mongo.Collection {
	return d.purchasesColl
}
