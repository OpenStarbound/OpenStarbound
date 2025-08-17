# chat

The new *chat* table is accessible from almost every clientside script and allows interaction with the in-game chat system.

---

#### `double` chat.send(`String` message, [`String` modeName], [`bool` speak], [`Json` data]))

Sends a new message in the chat. Optionally specifies the modeName:

- `"Broadcast"`: Global chat. Default value.
- `"Local"`: Chat within the current planet.
- `"Party"`: Chat within the current party.

If `speak` is false, the chat bubble will not appear above the player.
If `data` is provided, it will be sent as a JSON object with the message. This can be used to send custom data with the message.

---

#### `String[]` chat.command(`String` command)

Executes the specified command and returns a list of strings with the result.

---

#### `void` chat.addMessage(`String` text, [`Json` config])

Adds the specified message to the chat log. The following keys are available in the `config` JSON object:

- `String` __mode__ - The mode of the message. Can be one of the followgin: 
	- `"Broadcast"`
	- `"Local"`
	- `"Party"`
	- `"Whisper"`
	- `"CommandResult"`
	- `"RadioMessage"`
	- `"World"`
- `String` __channelName__ - The name of the channel to send the message to.
- `String` __fromNick__ - The name of the sender of the message.
- `String` __portrait__ - message portrait.
- `bool` __showPane__ - If false, the chat pane will not be triggered.

---

#### `String` chat.input()

Returns the current chat input text.

---

#### `bool` chat.setInput(`String` text, [`bool` moveCursor])

Sets the current chat input text. If `moveCursor` is true, the cursor will be moved to the end of the text. Returns true if the input was set successfully, false otherwise.

---

#### `void` chat.clear([`unsigned`] count)

Clears the chat input text. If `count` is provided, it will clear the last `count` messages, all otherwise.
