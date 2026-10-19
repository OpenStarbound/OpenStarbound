# Scripting Windows

Some native windows can be given scripts.
Add a `"scripts"` array and, if the script wants the native widgets' callbacks, `"scriptWidgetCallbacks"`, to the window's config with a patch.
Then, the window becomes a script pane.
It has `widget`, `pane`, `config`, `player`, `status` and `celestial`.
It runs `init`, `update(dt)` and `uninit` like any other script pane.
The behavior of a window whose config has no `"scripts"` is unchanged.

Note that the script runs after the window's own native code, either every frame (for the team bar and member menu) or when the window is shown (the others), so what it sets wins.

| Window | Config | Script callbacks |
|---|---|---|
| Identity panel | `/interface/windowconfig/teambar.config` | `update(dt)` |
| Incoming party invitation | `/interface/windowconfig/teaminvitation.config` | `invited(inviterName, inviterUuid)` |
| "Wishes to join your game" | `/interface/windowconfig/joinrequest.config` | `request(userName, message)` |
| Invite player | `/interface/windowconfig/teaminvite.config` | `invite(name)`: return a string to invite someone else, or `false` to stop the invitation |
| Party member menu | `/interface/windowconfig/teammembermenu.config` | `opened(memberUuid, memberName)`, and `update(dt)` after the native update every frame |

The widgets to change are the ones named in each config's `paneLayout`, for example `name`, `portrait`, `backing`, `overlay` and `leader` in the team bar, or `inviterName`, `titleLabel` and `inviteLabel` in the invitation. Any of them can be changed with the `widget` table (`widget.setText`, `widget.setImage`, `widget.setVisible`, `widget.setPosition`, etc.), and more can be added with `pane.addWidget`.

Windows that can be dismissed with the close key can be dismissed by the script with `pane.dismiss()`.

---

#### The `teamBar` Table

The team bar's portrait normally shows the player in `"bust"` mode.

##### `void` teamBar.setPortraitEntity([`EntityId` entityId])

Shows another entity's portrait. If called with no argument, resets to the default.

##### `void` teamBar.setPortraitMode(`String` mode)

`"head"`, `"bust"`, `"full"`, `"fullneutral"`, `"fullnude"` or `"fullneutralnude"`.

##### `void` teamBar.setPortraitScale(`float` scale)

---

For a portrait that is not an entity at all, hide it with `widget.setVisible("portrait", false)` and put an image where it was with `pane.addWidget` or by changing the `backing` / `overlay` image with `widget.setImage`.

---

#### The Party Portrait and Name

The name and portrait the rest of the party sees for you come from the player, not from the window, and are set with `player.setTeamName` and `player.setTeamPortrait`.
The name is a string (at most 64 characters, an empty one is ignored) shown in place of your name in everybody's team bar and member menu.

```lua
player.setTeamName("^#ff9ad5;Someone Else")
```

The portrait is a list of drawables.
Similarly to the name, it is sent with your other party data so everyone in the party sees it in their team bar.

```lua
player.setTeamPortrait({
  { image = "/items/generic/crafting/ironbar/ironbar.png", centered = true, scale = 1 }
})
```

The drawables are in portrait pixels, like the `head` portrait they replace, with `image` and optionally `position`, `scale`, `rotation`, `color`, `fullbright`.
Clear either with `nil`. They are not saved, so set them again whenever the script starts.
The images have to exist on the other players' games, so stick to vanilla assets unless they have your mod.
A list with more than 32 drawables, or one that cannot be read, raises an error.
