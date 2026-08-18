We need to think about how to split the 1000 or so cards within the game.
It would be great if the player could access 4 copies of every card in the game (except OP ones).

From each set, I will separate the cards into various tiers. I will make sure that there are the same amount of cards per tier in each civilization.

Mercer has his stock that builds as the player collects shards. Each town has its own civilization-specific shop. Also, each duelist gives a card reward upon defeat.


## Card Price Tiers

Prices are split into several tiers:
Tier 1: 100 gold
Tier 2: 500 gold
Tier 3: 1500 gold
Tier 4: 5000 gold
Tier 5: 25000 gold


## Mercer

Mercer is the card merchant. He sells cards, and his inventory expands once you give him shards.
This files contains Mercer's stock, and how it evolves upon giving him shards.
Mercer has unlimited copies of each card in his stock, but the player can only buy upto 4 of each card.
There are other traders that can be found in the overworld too. Their stock will also be listed here.

The in-world behavior of shards and Mercer's restoration bench is defined in
`Notes/WorldPhysics.md`. Shards calibrate compatible stock; they do not create
cards from nothing or contain trapped creatures.

### Initial Stock

Initially Mercer has 6 basic cards from each civilization. Total: 30 cards.

Artisan Picora
Brawler Zyler
Galsaur
Armored Walker Urherion
Draglide
Tornado Flame

Wandering Braineater
Masked Horror, Shadow of Scorn
Swamp Worm
Dark Clown
Gigargon
Creeping Plague

Hunter Fish
King Coral
Aqua Vehicle
Tropico
Marine Flower
Crystal Memory

Miele, Vizier of Lightning
Chilias, the Oracle
Dia Nork, Moonlight Guardian
Lah, Purification Enforcer
Senatine Jade Tree
Logic Sphere

Poisonous Mushroom
Fear Fang
Forest Hornet
Storm Shell
Stampeding Longhorn
Aura Blast

### Shards

Each shard gives 10 extra cards in his stock. Lets have 20 shards in Act I-II and 10 more shards in Act III. Total: 300 cards.


Furnace Shard: (found early, in Old Road)
Armored Cannon, Balbaro
Rothus, the Traveler
Explosive Dude Joe
Muramasa, Duke of Blades
Snip Striker Bullraizer
Blasto, Explosive Soldier
Wild Racer Chief Garan
Spastic Missile
Cyclone Panic
Magma Gazer

Blazing Shard:
Gatling Skyterror
Scarlet Skyterror
Boltail Dragon
Stonesaur
Dogarn, the Marauder
Armored Warrior Quelos
Flametropus
Burst Shot
Crisis Boulder
Energy Charger


Cataclysm Shard: (Act III)
Cyclone Panic
Cataclysmic Eruption
Legionnaire Lizard
Torchclencher
Valiant Warrior Exorious
Baby Zoppe
Kip Chipotto
Kipo's Contraption
Energy Charger
Otherworldly Warrior Naglu

Nightmare Shard: (found between Gloam Quarry and Cinderrail)
Horrid Worm
Chaos Worm
Amber Piercer
General Dark Fiend
Scratchclaw
Gigakail
Skullcutter, Swarm Leader
Lone Tear, Shadow of Solitude
Grim Soul, Shadow of Reversal
Critical Blade


Spectral Shard:
Shadow Moon, Cursed Shade
Skeleton Thief, the Revealer
Mongrel Man
Giriel, Ghastly Warrior
Vile Mulder, Wing of the Void
Scalpel Spider
Three-Faced Ashura Fang
Sinister General Damudo
Zorvaz, the Bonecrusher
Tank Mutant

Grave Shard: (Act III)
Battleship Mutant
Crath Lade, Merciless King
Gezary, Undercover Doll
Slash Charger
Grinning Hunger
Gabzagul, Warlord of Pain
Acid Reflux, the Fleshboiler
Gigamente
Gigandura
Spinal Parasite


Growth Shard: (found early, in Crossroads)
Barkwhip, the Smasher
Charmilia, the Enticer
Slumber Shell
Brood Shell
Launch Locust
Coliseum Shell
Solid Horn
Mystic Inscription
Muscle Charger
Aurora of Reversal


Mountain Shard: (Giants)
Bloodwing Mantis
Dawn Giant
Avalanche Giant
Mighty Bandit, Ace of Thieves
Silvermoon Trailblazer
Xeno Mantis
Fortress Shell
Sword Butterfly
Carbonite Scarab
Cannon Shell

Earthquake Shard: (Act III)
Clobber Totem
Whispering Totem
Whip Scorpion
Stinger Horn, the Delver
Tangle Fist, the Weaver
Senia, Orchard Avenger
Legacy Shell
Scowling Tomato
Thirst for the Hunt
Shaman Broccoli



Protector Shard: (found early, in Old Road)
Laguna, Lightning Enforcer
Raza Vega, Thunder Guardian
Dia Nork, Moonlight Guardian
Szubs Kin, Twilight Guardian
Reusol, the Oracle
Senatine Jade Tree
Larba Geer, the Immaculate
Alek, Solidity Enforcer
Kanesill, the Explorer
Logic Cube

Solar Shard: (Seekers)
Gran Gure, Space Guardian
Ra Vu, Seeker of Lightning
Ur Pale, Seeker of Sunlight
Aless, the Oracle
Fu Reil, Seeker of Storms
Re Bil, Seeker of Archery
Dava Torey, Seeker of Clouds
Telitol, the Explorer
Glory Snow
Rain of Arrows

Heaven Shard: (Act III)
Razorpine Tree
Vess, the Oracle
Yuluk, the Oracle
Laveil, Seeker of Catastrophe
Gandar, Seeker of Explosions
Pulsar Tree
Rom, Vizier of Tendrils
Solar Grass
Lazer Whip
Justice Jamming


Aqua Shard: (found early, in Crossroads)
Prowling Elephish
Pokolul
Aeropica
Aqua Rider
Raptor Fish
Madrillon Fish
Steam Star
Aqua Agent
Titanium Cluster
Crystal Paladin

Deep Shard: (Leviathans)
King Benthos
King Depthcon
King Mazelan
Aqua Grappler
Aqua Ranger
Mystic Dreamscape
Midnight Crawler
Impossible Tunnel
Kelp Candle
Marching Motherboard

Knowledge Shard: (Act III, Water cards that deal with information)
Vikorakys
Grape Globbo
Lalicious
Miracle Quest
Kyuroro
Candy Cluster
Submarine Project
Liquid Scope
Tekorax
Stinger Ball


Additional non-mono-color shards in Act III:

Rainbow Shard: (Multi-color cards)
Electro explorer Syrion
Estol, Vizier of Aqua
Melnia, the Aqua Shadow
Aqua Skydiver
Techno Totem
Soderlight, the Cold Blade
Tagtapp, the Retaliator
Sea Mutant Dormel
Spectral Horn Glitalis
Buzz Betocchi

Fury Shard: (Wave Striker, and utility abilities)
Saliva Worm
Lamiel, Destiny Enforcer
Klujadras
Skyscraper Shell
Flame Trooper Goliac
Sky Crusher, the Agitator
Spinning Totem
Gachack, Mechanical Doll
Heavyweight Dragon
Tra Rion, Penumbra Guardian

Eternal Shard: (Silent skills, utility abilities etc)
Gankloak, Rogue Commando
Lockdown Lizard
Hustle Berry
Venom Capsule
Kejila, Hidden Horror
Minelord Skyterror
Vorg's Engine
Kaemira, the Oracle
Milporo
Charge Whipper

Colossal Shard: (Some very strong cards)
Ten-Ton Crunch
Jabaha's Automaton
Gigaslug
Evil Incarnate
Radioactive Horn, the Strange
Fever Nuts
Cosmic Darts
Kilstine, Nebula Elemental
Warped Lunatron
Lucky Ball

Imperial Shard: (Some more strong cards)
Roulette of Ruin
Royal Durian
Rainbow Gate
Morbid Medicine
Hypersprint Warior Uzesol
Muramasa's Knife
Supersonic Jetpack
Rapid Reincarnation
Wind Axe, the Warrior Savage
Obsidian Scarab




## Town Shops

Each civilization's core town has its own shop with 15 cards. This shop expands by another 15 cards when the player reaches Act III. With five core shops, the total core inventory is 150 cards. Specialist settlements such as Tidevault and Stonechant add focused stock outside that count and may deliberately overlap a few regional staples.

### Cinderrail

Initial stock:
Immortal Baron, Vorg
Mini Titan Gett
Meteosaur
Bombersaur
Nomad Hero Gigio
Super Explosive Volcanodon
Onslaughter Triceps
Deadly Fighter Braid Claw
Fire Sweeper Burning Hellion
Explosive Fighter Ucarn
Fatal Attacker Horvath
Cavalry General Curatops
Burning Power
Chaos Strike
Rumble Gate

Act III Bonus:
Pyrofighter Magnus
Rikabu, the Dismantler
Bombat, General of Speed
Rikabu's Screwdriver
Picora's Wrench
Kamikaze, Chainsaw Warrior
Missile Boy
Chaotic Skyterror
Cratersaur
Magmarex
Cannoneer Bargon
Mega Detonator
Volcanic Arrows
Searing Wave
Volcanic Charger


### Rootmaze

Initial stock:
Burning Mane
Thorny Mandra
Poisonous Dahlia
Red-eye Scorpion
Tower Shell
Golden-Wing Striker
Mighty Shouter
Essence Elf
Elf-X
Silver Axe
Sniper Mosquito
Raging Dash-horn
Mana Crisis
Rainbow Stone
Ultimate Force

Act III Bonus:
Masked Pomegranate
Dew Mushroom
Exploding Cactus
Torcon
Three-eyed Dragonfly
Supporting Tulip
Scissor Scarab
Rumbling Terahorn
Garabon, the Glider
Paradise Horn
Carrier Shell
Dimension Gate
Roar of the Earth
Enchanted Soil
Mulch Charger

### Glasswater

Initial stock:
Aqua Soldier
Aqua Shooter
Candy Drop
Faerie Child
Phantom Fish
Plasma Chaser
Aqua Bouncer
Saucer-Head Shark
Unicorn Fish
Chaos Fish
Aqua Guard
Aqua Jolter
Virtual Tripwire
Teleportation
Recon Operation

Act III Bonus:
Hunter Cluster
King Aquakamui
Keeper of the Sunlit Abyss
Smile Angler
Sea Slug
Lurking Eel
Solidskin Fish
Neon Cluster
Overload Cluster
Sopian
Thrash Crawler
Marine Scramble
Wave Lance
Shock Hurricane
Riptide Charger

### Tidevault

Tidevault opens with the outer network and carries advanced Water cards that
support the late Water Seal chapter. Its inventory combines new finishers with
selected Glasswater staples so a player is not forced to exhaust both shops.

Initial late-game stock:
Crystal Lancer
Aqua Surfer
Corile
Illusionary Merfolk
King Tsunami
King Depthcon
Hydrooze, the Mutant Emperor
Angler Cluster
Hazard Crawler
Thrash Crawler
Marine Scramble
Shock Hurricane
Riptide Charger

After restoring the Water Seal:
Crystal Paladin
Legendary Bynor
King Aquakamui
Keeper of the Sunlit Abyss
Sopian
Overload Cluster
Wave Lance

### Stonechant Village

Stonechant's specialist stall is the main source for Mystery Totem decks. The
early list provides enough repeated bodies and mana support to make a coherent
deck, while stronger build-around Totems unlock through local challenges and
later story progress.

Initial stock:
Jiggly Totem
Whispering Totem
Shaman Broccoli
Spinning Totem
Techno Totem
Carnival Totem
Dimension Gate
Enchanted Soil
Mulch Charger

Local challenge and late stock:
Clobber Totem
Bliss Totem, Avatar of Luck
Forbidding Totem
Vreemah, Freaky Mojo Totem
Dracodance Totem
Rollicking Totem
Cursed Totem

Cryptic Totem remains associated with Oren's Verdant Crest reward rather than
being sold on the first visit.

### Gloam Quarry

Initial Stock:
Bloody Squito
Gigagiele
Gigaberos
Marrow Ooze, the Twister
Poison Worm
Stinger Worm
Writhing Bone Ghoul
Bone Assassin, the Ripper
Bone Spider
Gigastand
Dark Titan Maginn
Gray Balloon, Shadow of Greed
Wailing Shadow Belbetphlo
Death Smoke
Dark Reversal


Act III Bonus:
Purple Piercer
Gigabuster
Gigaclaws
Gigabolver
Gregorian Worm
Volcano Smog, Deceptive Shade
Wisp Howler, Shadow of Tears
Jewel Spider
Cursed Pincher
Lupa, Poison-Tipped Doll
Horned Mutant
Zombie Carnival
Dark Pact
Venom Charger
Hopeless Vortex


### Stormbreak

Initial Stock:
Iere, Vizier of Bullets
Lok, Vizier of Hunting
Rayla, Truth Enforcer
Ruby Grass
Spiral Grass
Wyn, the Oracle
Reso Pacos, Clear Sky Guardian
Magris, Vizier of Magnetism
Fonch, the Oracle
Iocant, the Oracle
Kulus, Soulshine Enforcer
La Ura Giga, Sky Guardian
Laser Wing
Moonlight Flash
Solar Ray

Act III Bonus:
Phal Eega, Dawn Guardian
Amber Grass
Kolon, the Oracle
Sarius, Vizier of Suppression
Milieus, the Daystretcher
Mist Rias, Sonic Guardian
Gulas Rias, Speed Guardian
Ouks, Vizier of Restoration
Chen Treg, Vizier of Blades
Lightning Grass
Lu Gila, Silver Rift Guardian
Sundrop Armor
Boomerang Comet
Full Defensor
Lightning Charger


### Clayhearth

This place has various survivor cards.

Inital Stock:
Blazosaur Q
Spikestrike Ichthys Q
Split-head Hydroturtle Q
Gigaling Q
Skullsweeper Q
Ballus, Dogfight Enforcer Q
Gallia Zohl, Iron Guardian Q
Balloonshroom Q

Act III Bonus:
Forbos, Sanctum Guardian Q
Ripple Lotus Q
Rumblesaur Q
Factory Shell Q
Grave Worm Q
Promephius Q
Bladerush Skyterror Q
Smash Horn Q

### Dragon Keep

There will be three shops in this city. One shop sells dragons, another sells Angel Commands, and another sells Demon Commands.

Dragon Shop, first inventory:
Bolzard Dragon
Galklife Dragon
Bruiser Dragon
Gazarias Dragon
Garkago Dragon
Gaulezal Dragon
Uberdragon Jabaha
Magmadragon Melgars
Necrodragon Zalva
Necrodragon Giland
Necrodragon Galbazeek
Terradraon Regarion
Terradragon Cusdalf
Terradragon Gamiratar
Furious Onslaught
Phantom Dragon's Flame

Dragon Shop, more cards after beating Kestrel:
Bazagazeal Dragon
Bolgash Dragon
Terradragon Dakma Balgarow
Terradragon Arque Delacerna
Dracodance Totem
Rollicking Totem
Kachua, Keeper of the Icegate
Kryston, Lair Delver
Pippie Kuppie
Mechadragon's Breath
Bakkra Horn, the Silent
Super Terradragon, Bailas Gale

Demon Shop:
Gamil, Knight of Hatred
Gnarvash, Merchant of Blood
Photocide, Lord of the Wastes
Vashuna, Sword Dancer
Gajirabute, Vile Centurion
Deathliger, Lion of Chaos
Zagaan, Knight of Darkness

Angel Shop:
Ethel, Star Sea Elemental
Aeris, Flight Elemental
Mihail, Celestial Elemental
Rimuel, Cloudbreak Elemental
Miar, Comet Elemental
Gariel, Elemental of Sunbeams
Syforce, Aurora Elemental
Siri, Glory Elemental
Elixia, Pureblade Elemental
Hanusa, Radiance Elemental
Urth, Purifying Elemental
Dracobarrier
Wave Lance

### Cloudrest Peak

The place has Wave striker cards:
Asra, Vizier of Safety
Jagila, Hidden Pillager
Steamroller Mutant
Eviscerating Warrior Lumez
Ninja Pumpkin
Uncanny Turnip
Revival Soldier
Merlee, the Oracle
Aqua Trickster
Bonfire Lizard
Macho Melon
Hazaria, Duke of Thorns

## Sunspire Cloister

Misha, Channeler of Suns
Nastasha, Channeler of Suns
Sasha, Channeler of Suns
Petrova, Channeler of Suns
Yuliana, Channeler of Suns

## Special Cards

Certain cards can only be accessed upon defeating opponents or in secrets. These are:

Crimson Hammer/Comet Missile: 4 early opponents give out this card

Bronze-Arm Tribe/Faerie Life: 4 early opponents

Aqua Hulcus/Energy Stream: 4 early opponents

Ghost Touch/Horrid Worm/Terror Pit: 4 mid-tier opponents

Spiral Gate/Aqua Surfer: Mid-tier opponents

Diamond Cutter/Holy Awe: Mid-tier

Mana Nexus: mid-tier

Bolshack Dragon/Metalwing Skyterror: Mid-tier opponents

Locomotiver: High-tier

Cocco Lupia/Totto Pipicchi: Various opponents in Dragon Keep

Bolmeteus Steel Dragon/Twin-Cannon Skyterror: Special opponents in Dragon Keep

Future Slash/Apocalypse Vise/Lost Soul: special opp

Magmadragon Jagalzor: Special opponents in Dragon Keep

Trox, General of Destruction: Special opponents in Gloam quarry and Dragon Keep

Balloom, Master of Death/Alcadeias, Lord of Spirits: Special opponents

Stallob/Death Cruzer: special battles

Divine Riptide/Mana Bonanza: Special opp

Sirius, Firmament Elemental: Special opp

Q-tronix Hypermind/Gargantua/Omnistrain: Two duelists in Cinderrail and adjacent areas give out these cards. Omnistrain is more of a secret card.

Sword of Malevolent Death/Benevolent Life: An NPC gives you either one of the two, after a philosophical riddle

Invincibles: Special cards, can be found in a chest after a particularly tough battle in each civilization's dungeon

Uberdragon Bajula: Secret opponent in Dragon Keep after beating Kestrel

Ultimate Dragon: Drops from Kestrel

Billion-degree Dragon: Secret opponent in Dragon Keep after beating Kestrel

Kuukai: Special drop

La Byle: special

Ailzonius: special opponent

Super Necrodragon, Abzo Dolba: Special opponent in Dragon Keep

All vortex evolutions are special opponents, and so are all multi-race evolutions

Multi-color spells are special opponents

All Channelers: Special shop in Sunspire

Cranium Clamp: You get only 1 copy in a special place in Act III

Bombazar, Dragon of Destiny: Only 1 copy in a secret location after beating Caelum Rhos

### Weaker card drops

Special:
Innocent Hunter, Blade of All

Dragons: (Dragon Keep drops)
Necrodragon Jagraveen
Dimension Splitter
Kooc Pollon
Scream Slicer, Shadow of Fear (dragonoid/dragon)

Darkness Creatures:
Bazooka Mutant
Propeller Mutant
Gigagriff
Tentacle Worm
Tyrant Worm
Junkatz, Rabid Doll
Trixo, Wicked Doll
Grinning Axe, the Monstrosity
Dream Pirate, Shadow of Theft
Ice Vapor, Shadow of Anguish
Bat Doctor, Shadow of Undeath

Dark Lords:
Schuka, Duke of Amnesia
Megaria
Baraga, Blade of Gloom
Gregoria, Princess of War
Gabzagul, Warlord of Pain

Darkness Spells:
Eldritch Poison
Slime Veil (Dark-Light decks)
Ghastly Drain
Dark Pact
Scheming Hands
Soul Gulp
Proclamation of Death
Vacuum Gel
Corpse Charger

Nature Creatures: 
Crow Winger
Feather Horn, the Tracker
Moon Horn
Trench Scarab
Ambush Scorpion
Spliterclaw Wasp
Illusory Berry
Psyshroom
Pouch Shell
Bliss Totem, Avatar of Luck
Forbidding Totem
Vreemah, Freaky Mojo Totem
Poppel, Flowerpetal Dancer
Cavern Raider
Quixotic Hero Swine Snout

Nature Spells:
Mystic Treasure Chest
Freezing Icehammer
Fruit of Eternity
Root Charger
Dance of the Sproutlings (race evo decks)
Vine Charger

Giants:
Cliffcrush Giant
Earthstomp Giant
Ancient Giant
Nocturnal Giant
Cantankerous Giant

Water Creatures: 
Steel-Turret Cluster
Scout Cluster
Tentacle Cluster
Zepimeteus
Stained Glass
Scissor Eye
Shtra
Zepellin Crawler
Hazard Crawler
Aqua Fencer
Aqua Master
Garatyano
Illusion Fish
Hokira

Water Spells: 
Thought Probe
Hydro Hurricane
Flood Valve
Abduction Charger

Leviathans:
King Triumphant
King Neptas
King Ponitas

Light Creatures: 
Calgo, Vizier of Rainclouds
Lena, Vizier of Brilliance
Chekicul, Vizer of Endurance
Migalo, Vizier of Spycraft
Ballas, Vizier of Electrons
Kalute, Vizier of Eternity
Sparkle Flower
Sol Galla, Halo Guardian
Jil Warka, Time Guardian
Thrumiss, Zephyr Guardian
Adomis, the Oracle
Nariel, the Oracle
Bex, the Oracle
Micute, the Oracle
Moontear, Spectral Knight
Geoshine, Spectral Knight
Cyclolink, Spectral Knight
Cosmogold, Spectral Knight
Betrale, the Explorer

Light Spells:
Sphere of Wonder
Whisking Whirlwing
Protective Force
Screaming Sunburst
Thunder Net (Water-Light decks)
Miracle Portal
Lunar Charger
Laser Whip
Nexus Charger
Cosmic Wing
Unified Resistance (evo race)

Fire Creatures:
Automated Weaponmaster Machai
Armored Scout Gestuchar
Badlands Lizard
Migasa, Adept of Chaos
Choya, the Unheeding
Wild Racer Chief Garan
Astronaut Skyterror
Torpedo Skyterror
Cutthroat Skyterror (speed attacker)
Aerodactyl Kooza
Steam Rumbler Kain
Quakesaur
Missile Soldier Ultimo
Snaptoungue Lizard
Shock Trooper Mykee (speed attacker)

Fire Spells:
Blaze Cannon
Fists of Forever
Relentless Blitz
Blizzard of Spears
