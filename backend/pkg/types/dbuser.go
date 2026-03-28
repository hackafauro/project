package types

import (
	"encoding/json"
	"time"

	"go.mongodb.org/mongo-driver/v2/bson"
	"go.mongodb.org/mongo-driver/v2/mongo"
)

type DBUser struct {
	ID               bson.ObjectID `bson:"_id,omitempty"`
	Name             string        `bson:"name" json:"name"`
	Surname          string        `bson:"surname" json:"surname"`
	Email            string        `bson:"email" json:"email"`
	Password         string        `bson:"hash" json:"-"`
	DiscountDate     time.Time     `bson:"discount" json:"-"`
	DiscountEarned   bool          `bson:"discountEarned" json:"discountEarned"`
	RecentSearches   []string      `bson:"recent" json:"recent"`
	DiscountPlayable bool          `bson:"-" json:"discountPlayable"`
}

func UserFromToken(token string, collection *mongo.Collection) *DBUser {
	query := bson.M{"hash": token}
	var user DBUser
	err := collection.FindOne(nil, query).Decode(&user)
	if err != nil {
		return nil
	}

	return &user
}

func UserFromId(id string, collection *mongo.Collection) *DBUser {
	query := bson.M{"_id": id}
	var user DBUser
	err := collection.FindOne(nil, query).Decode(&user)
	if err != nil {
		return nil
	}

	return &user
}

func (user *DBUser) IsDiscountPlayable() bool {
	return time.Since(user.DiscountDate) > 24*time.Hour
}

func (user *DBUser) ToAPI() []byte {
	userCopy := *user
	userCopy.DiscountPlayable = false
	if userCopy.IsDiscountPlayable() {
		userCopy.DiscountEarned = false
		userCopy.DiscountPlayable = true
	}
	j, err := json.Marshal(userCopy)
	if err != nil {
		return nil
	}

	return j
}

func (user *DBUser) GameResult(win bool, collection *mongo.Collection) error {
	user.DiscountDate = time.Now()
	user.DiscountEarned = win

	_, err := collection.UpdateOne(nil, bson.M{"_id": user.ID}, bson.M{"$set": bson.M{
		"discount":       user.DiscountDate,
		"discountEarned": user.DiscountEarned,
	}})

	return err
}
