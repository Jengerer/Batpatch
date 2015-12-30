# Batpatch
An unofficial patcher to get Batman: Arkham City GOTY Edition to execute as the non-GOTY version on Steam.

The main purpose of doing so is if you'd like to finish some achievements on the non-GOTY version, but are unable to launch it any more due to being unable to activate it.
It works by patching the GOTY version's executable to use the non-GOTY version's App ID (and patches out code that verifies the executable).
After patching, you *MUST* launch the game through the .exe directly, as launching through Steam will force it to use the GOTY version's App ID, and then the game will try to re-launch the non-GOTY version since it sees mismatching IDs between the executable and what's running.