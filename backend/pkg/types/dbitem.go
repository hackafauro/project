package types

import (
	"context"
	"errors"

	"go.mongodb.org/mongo-driver/v2/bson"
	"go.mongodb.org/mongo-driver/v2/mongo"
)

type DBReview struct {
	Rating int           `bson:"rating" json:"rating"`
	Text   string        `bson:"text" json:"text"`
	User   bson.ObjectID `bson:"user" json:"user"`
}

type DBProduct struct {
	ID          bson.ObjectID `bson:"_id,omitempty" json:"ID"`
	Name        string        `bson:"name" json:"name"`
	Tags        []string      `bson:"tags" json:"tags"`
	Price       float64       `bson:"price" json:"price"`
	Description string        `bson:"description" json:"description"`
	Reviews     []DBReview    `bson:"reviews" json:"reviews"`
	Image       string        `bson:"image" json:"image"`

	AiWhy string `bson:"-" json:"aiWhy"`
}

func GetAllTags(appState *AppContext, collection *mongo.Collection) []string {
	if tags := appState.GetTags(); tags != nil {
		return tags
	}

	ctx := context.Background()
	pipeline := mongo.Pipeline{
		{{Key: "$unwind", Value: "$tags"}},
		{{Key: "$group", Value: bson.M{"_id": "$tags"}}},
		{{Key: "$sort", Value: bson.M{"_id": 1}}},
	}

	cursor, err := collection.Aggregate(ctx, pipeline)
	if err != nil {
		return []string{}
	}
	defer cursor.Close(ctx)

	var results []struct {
		ID string `bson:"_id"`
	}
	if err = cursor.All(ctx, &results); err != nil {
		return []string{}
	}

	tags := make([]string, 0, len(results))
	for _, r := range results {
		tags = append(tags, r.ID)
	}

	appState.SetTags(tags)

	return tags
}

func GetProductsByTag(tags []string, collection *mongo.Collection) []DBProduct {
	products := make([]DBProduct, 0)

	cursor, err := collection.Find(nil, bson.M{"tags": bson.M{"$in": tags}})
	if err != nil {
		if errors.Is(err, mongo.ErrNoDocuments) {
			return []DBProduct{}
		}
		return nil
	}

	if err = cursor.All(nil, &products); err != nil {
		return nil
	}

	return products
}

func GetProductByID(id string, collection *mongo.Collection) *DBProduct {
	var product DBProduct
	obj, err := bson.ObjectIDFromHex(id)
	if err != nil {
		return nil
	}
	err = collection.FindOne(nil, bson.M{"_id": obj}).Decode(&product)
	if err != nil {
		return nil
	}

	return &product
}
