# Client mesages to server with server responses

## JOIN

From client:

```
[MessageType::kJoin, PlayerId]
```
From server:

Success:
```
[MessageType::kWelcome]
```

Failed:
```
[MessageTy
pe::kAuthFailed, PlayerId]
```

## RECONNECT

From client:

```
[MessageType::kReconnect, PlayerId]
```
From server:

Success:
```
[MessageType::kReconnected]
```

Failed:
```
[MessageType::kNotReconnected]
```

## CREATE_LOBBY

From client:

```
[MessageType::kCreateLobby]
```
From server:

Success:
```
[MessageType::kLobbyCreated, LobbyId]
```

Failed:
```
[MessageType::kLobbyNotCreated]
```

## LIST_LOBBY

From client:

```
[MessageType::kListLobby]
```
From server:

Success:
```
[MessageType::kLobbyListMessage, LobbyList]
```

Failed:
```
[MessageType::kLobbyListFailedMessage]
```

## JOIN_LOBBY

From client:

```
[MessageType::kJoinLobby, LobbyId] 
```
From server:

Success:
```
[MessageType::kPlayerJoinedLobby]
```

Failed:
```
[MessageType::kLobbyDoesNotExist]
```
```
[MessageType::kLobbyFull]
```

## LEAVE_LOBBY

From client:

```
[MessageType::kLeaveLobby]   
```
From server:

Success:
```
[MessageType::kPlayerLeftLobby]
```

Failed:
```
[MessageType::kPlayerNotConsistsInLobby]
```

## START_GAME

From client:

```
[MessageType::kStartGame]  
```
From server:

Success:
```
[MessageType::kGameStarted, GameId]
```

Failed:
```
[MessageType::kLobbyNotReady]
```
```
[MessageType::kNotTheLeader]
```

## PING

From client:

```
[MessageType::kPing]
```
From server:

```
[MessageType::kPong]
```

# Client messages to server without server responses

## MOVE

From client:

```
[MessageType::kMove, Direction]
```

## ATTACK

From client:

```
[MessageType::kAttack]
```
# Server messages to client without client responses

## STATE_UPDATE

From server:

```
[MessageType::kStateUpdate, GameField]
```

* GameField:
```
Players: {{id, pos, health}, ...}
Mobs: {{id, pos, health}, ...}
```

## GAME_OVER

From server:

```
[MessageType::kGameOver, GameResult]
```
