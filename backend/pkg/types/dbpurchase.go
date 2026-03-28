package types

import (
	"encoding/json"
	"errors"
	"time"

	"go.mongodb.org/mongo-driver/v2/bson"
	"go.mongodb.org/mongo-driver/v2/mongo"
)

type DBPurchase struct {
	ID     bson.ObjectID `bson:"_id,omitempty"`
	UserID bson.ObjectID `bson:"user" json:"-"`
	ItemID bson.ObjectID `bson:"item" json:"-"`
	Price  float64       `bson:"price" json:"price"`
	Date   time.Time     `bson:"date" json:"date"`

	ItemName  string `bson:"-" json:"itemName"`
	ItemImage string `bson:"-" json:"itemImage"`
}

func (purchase *DBPurchase) FetchItemData(items *mongo.Collection) error {
	var item DBProduct
	err := items.FindOne(nil, bson.M{"_id": purchase.ItemID}).Decode(&item)
	if err != nil {
		return err
	}

	purchase.ItemName = item.Name
	purchase.ItemImage = item.Image

	return nil
}

func PurchasesFromToken(hash string, users *mongo.Collection, purchases *mongo.Collection) []DBPurchase {
	user := UserFromToken(hash, users)
	if user == nil {
		return nil
	}

	cursor, err := purchases.Find(nil, bson.M{"user": user.ID})
	if err != nil {
		if errors.Is(err, mongo.ErrNoDocuments) {
			return []DBPurchase{}
		}
		return nil
	}

	var res []DBPurchase
	if err = cursor.All(nil, &res); err != nil {
		return nil
	}

	var successFullItems = make([]DBPurchase, 0)
	for i := range res {
		if err = res[i].FetchItemData(purchases); err == nil {
			successFullItems = append(successFullItems, res[i])
		}
	}

	return successFullItems
}

func PurchasesToApi(purchases []DBPurchase) []byte {
	j, err := json.Marshal(purchases)
	if err != nil {
		return nil
	}

	return j
}
