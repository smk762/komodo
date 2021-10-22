# Tokel blockchain code commit process/flow

## Repository overview

### TokelPlatform/komodo

#### Tkltest branch

https://github.com/TokelPlatform/komodo/tree/tkltest

Used to test blockchain code in a community environment using the TKLTEST (test net) blockchains.

#### Tokel branch

https://github.com/TokelPlatform/komodo/tree/tokel

This is the Tokel ‘master’ branch. The Tokel chain runs off this branch.

This branch is to be kept identical to the KomodoPlatform/Komodo ‘tokel’ branch. 

### KomodoPlatform/Komodo

#### Tokel branch

https://github.com/KomodoPlatform/komodo/tree/tokel

This branch is used by Komodo Notary Nodes. This branch is a ‘gateway’ between the Komodo & Tokel branches. All Tokel development is pushed through here, and vice versa for Komodo development/updates.

## The Process - working on the Tokel Blockchain features

Feature development -> Team/Community testing -> PR to the `tokel` branch

## Feature development

1. Fork this repository and make a new feature branch. 
2. Once done make a PR to TokelPlatform/komodo ‘tkltest’.
3. Reach out to someone to review your code.

## Team/community testing

After getting you PR reviewed and merge to `tkltest` Team/community performs through testing.

## Accepted/tested code

If all the tests have gone well, the resulting code is PRd into these repositories:
1. `TokelPlatform/Komodo` `tokel` branch
2. `KomodoPlatform/Komodo` `tokel` branch

## Updates coming from Komodo side

1. PR from a branch in Komodo repository


## Updates coming from Komodo side
