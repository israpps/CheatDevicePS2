---
sort: 4
---

# Value Mapped Cheats

A value-mapped cheat allows a cheat to reference a list of possible values without duplicating the cheat for each value. For example, a game's inventory system might use an array of bytes to represent the contents of each slot, where the slot's value represents a unique item. Rather than creating separate cheats for each combination of slot and item, a single cheat can be defined for each slot using a reference to a value map with each item's digits. In Cheat Device, choosing the *Slot X* cheat displays a list of item descriptions, allowing any item's digits to be used with the cheat.

A value map is defined by a name enclosed by brackets followed by value-key pairs separated by a `=`, `-`, or `:` character. Values are in hexadecimal and can be either 2, 4, or 8 characters in length. A game can define up to 16 value maps that can be referenced by any of the game's code lines that appear after the map has been defined.

A code line can reference a value map by using a `$` character followed by the value map's name. The reference can appear in place of the usual value digits that would follow a code's address (i.e. `20000000 $MapName`), and can optionally be preceded by 2, 4, or 6 digits to be combined with the mapped value. Regardless of if the preceding digits are specified or not, the mapped value will always be used in place of the lower bits of the code value.

## Example

```
"Grand Theft Auto Vice City v1.40"

[Car]
82=Landstalker
83=Idaho
84=Stinger
85=Linerunner
86=Perennial
87=Sentinel
88=Patriot (Boat)

Press {SELECT} to Spawn a Car
10275F8C 0000001A
10275F9C 00000042
20275FB0 0C09E528
20275FB4 2404$Car
```

* A value map named `Car` with 7 entries is defined for `Grand Theft Auto Vice City v1.40`.
* A cheat named `Press {SELECT} to Spawn a Car` references the map in the 4th code line. When the cheat is chosen in Cheat Device, a list of all possible `Car` values to be used with the cheat is displayed.
* As an example, if *Stinger* is chosen from the list, `20275FB4 24040084` will be added to the list of codes to be processed by the code engine.

## Advanced Examples

```
"Game Title"
[Foo]
00 - Bar
01    =     Whitespace doesnt matter here
02: Item 3

All of these cheats are equivalent
Cheat 1
20000000 $Foo
Cheat 2
20000000 00$Foo
Cheat 3
20000000 0000$Foo
Cheat 4
20000000 000000$Foo

[AnotherMap]
1234 - Value 1
FFFF - Value 2
00FF - This value...
FF: Is the same as this one!

Cheat that uses 16-bit values
Cheat 1
10000000 $AnotherMap
Cheats 2
10000000 0000$AnotherMap
Cheat 3
20000000 1234$AnotherMap
Cheat 4
20000000 FF$AnotherMap

[32-BitValues]
12345678 - Big Value 1
87654321 - Big Value 2

Cheat that uses 32-bit values
20000000 $32-BitValues
```

```note
* Value map names cannot contain spaces.
* Different games can have value maps with the same name.
* A cheat with multipe code lines can use a value map, but only one of the lines can reference it.
* Value map entries are insensitive to whitespace around the separating character.
```
