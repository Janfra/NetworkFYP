# NetworkFYP
 Exploring creating a simple online multiplayer game using Unreal Engine 5.4.4

# Progress
## 1.0:
### FEATURES
- Player can create and join session via LAN using Unreal Engine session API
- Players can move and it replicates via `Movement Component`
- Players can shoot projectiles that replicate via the use of a `reliable` RPC and create local VFX on destroy
- Players have a `Health Component` that replicates their current health with a `RepNotify`

## 1.1:
### FEATURES
- Players can now die
- Players are respawn on death after a small delay on the server 
- Players ragdoll on death locally

### UI
- Players now have a local health bar on their `HUD` to show their health
- Enemy players now have a health bar on their head that uses the replicated health value from `Health Component`

## 1.2:
### NOTES
- Push Model: Attempted to add the push model when replicating, however in order to succesfully package, I would need to download source engine as I was unable to make an unique Build Environment, and overriding would give linker errors. Due to this, I had to remove it.

### FEATURES
- Players are now pushed by the projectiles 
- There are two teams, with each player being assigned to a team on `PlayerState` added
- Collectables were added as a way to earn score for your team
- Each team can reach a target score to win, however winning is not implemented yet

### UI
- Each team score is displayed in game, showing progress towards winning by reaching score goal

### KNOWN ISSUES
- Players are able to leave and teams will not be adjusted
- Spawning hasn't been adjusted per team