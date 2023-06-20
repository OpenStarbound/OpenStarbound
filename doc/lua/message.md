The message table contains a single function, setHandler, which allows entities to receive messages sent using world.sendEntityMessage. Entities which can receive messages include:

* monster
* NPC
* object
* vehicle
* stagehand
* projectile

Additionally, messages can be handled by a variety of script contexts that run on the player:

* activeitem
* quest
* playercompanions
* status

---

#### `void` message.setHandler(`String` messageName, `LuaFunction` handler)

Messages of the specified message type received by this script context will call the specified function. The first two arguments passed to the handler function will be the `String` messageName and a `bool` indicating whether the message is from a local entity, followed by any arguments sent with the message.
