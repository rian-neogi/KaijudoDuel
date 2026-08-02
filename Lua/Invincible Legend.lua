package.path = package.path .. ';./?.lua;'
require("Lua/Invincible Charge")

Cards["Nastasha, Channeler of Suns"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		if(getMessageType()=="mod breakshield" and getCardZone(id)==ZONE_BATTLE and getMessageInt("player")==getCardOwner(id)) then
			local ch = createChoiceNoCheck("Destroy Nastasha instead?",2,id,getCardOwner(id),Checks.False)
			if(ch==RETURN_BUTTON1) then
				setMessageInt("msgContinue",0)
				destroyCreature(id)
			end
		end
	end
}

Cards["Emperor Quazla"] = {
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Evolution(id,"Cyber Lord")

		if(getMessageType()=="post cardmove" and getCardZone(id)==ZONE_BATTLE and getMessageInt("from")==ZONE_SHIELD and getMessageInt("to")==ZONE_BATTLE) then
			local trigger = getMessageInt("card")
			if(getCardOwner(trigger)~=getCardOwner(id)) then
				for i=1,2 do
					local ch = createChoiceNoCheck("Draw a card?",2,id,getCardOwner(id),Checks.False)
					if(ch~=RETURN_BUTTON1) then
						break
					end
					drawCards(getCardOwner(id),1)
				end
			end
		end
	end
}

Cards["Super Necrodragon Abzo Dolba"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 3,

	HandleMessage = function(id)
		Abils.Evolution(id, "Dragon")

		if(getMessageType()=="get creaturepower") then
			if(getMessageInt("creature")==id) then
				local c = Functions.countInZone(id, getCardOwner(id), ZONE_GRAVEYARD, Checks.IsCreature)
				setMessageInt("power",getMessageInt("power")+c*2000)
			end
		end
	end
}

Cards["Uberdragon Bajula"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 3,

	HandleMessage = function(id)
		Abils.Evolution(id,"Dragon")
		Abils.destroyOppManaOnAttack(id,2)
	end
}

local bailasGaleTriggers = {}

Cards["Super Terradragon Bailas Gale"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.Evolution(id,"Dragon")

		local messageType = getMessageType()
		local card = getMessageInt("card")
		if(messageType=="post cardmove" and getCardZone(id)==ZONE_BATTLE and getMessageInt("from")==ZONE_SHIELD and getMessageInt("to")==ZONE_BATTLE and getCardType(card)==TYPE_SPELL and getCardOwner(card)==getCardOwner(id)) then
			bailasGaleTriggers[card] = getCardOwner(id)
		elseif(messageType=="mod cardmove" and bailasGaleTriggers[card]==getCardOwner(id) and getCardZone(id)==ZONE_BATTLE and getMessageInt("from")==ZONE_BATTLE and getMessageInt("to")==ZONE_GRAVEYARD) then
			setMessageInt("to",ZONE_HAND)
		elseif(messageType=="post cardmove" and bailasGaleTriggers[card]~=nil and getMessageInt("from")==ZONE_BATTLE) then
			bailasGaleTriggers[card] = nil
		end
	end
}

Cards["Kuukai, Finder of Karma"] = {
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Evolution(id,"Mecha Thunder")
		Abils.cantAttackPlayers(id)

		if(getMessageType()=="post creaturebattle" and getMessageInt("blocked")==1 and getMessageInt("defender")==id and getCardZone(id)==ZONE_BATTLE) then
			untapCard(id)
		end
	end
}

Cards["Aqua Ranger"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantBeBlocked(id)
		Abils.returnAfterDestroyed(id)
	end
}

Cards["Megaria, Empress of Dread"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="post creaturebattle" and getCardZone(id)==ZONE_BATTLE) then
			local attacker = getMessageInt("attacker")
			local defender = getMessageInt("defender")
			if(getCardZone(defender)==ZONE_BATTLE) then
				destroyCreature(defender)
			end
			if(getCardZone(attacker)==ZONE_BATTLE) then
				destroyCreature(attacker)
			end
		end
	end
}

Cards["Magmadragon Jagalzor"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		local turboRush = function(id)
			if(getMessageType()=="get creatureisspeedattacker") then
				local creature = getMessageInt("creature")
				if(getCardOwner(creature)==getCardOwner(id) and getCardZone(creature)==ZONE_BATTLE) then
					setMessageInt("isspeedattacker",1)
				end
			end
		end
		Abils.TurboRush(id,turboRush)
	end
}

Cards["Kachua, Keeper of the Icegate"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local valid = function(cid,sid)
			if(Checks.CreatureInYourDeck(cid,sid)==1 and isCreatureOfRace(sid,"Dragon")==1) then
				return 1
			end
			return 0
		end
		local tapAbility = function(id)
			local owner = getCardOwner(id)
			openDeck(owner)
			local ch = createChoice("Choose a Dragon in your deck",1,id,owner,valid)
			closeDeck(owner)
			if(ch>=0) then
				local mod = function(cid,mid)
					Abils.SpeedAttacker(cid)
					if(getMessageType()=="pre endturn" and getMessageInt("player")==getCardOwner(cid)) then
						if(getCardZone(cid)==ZONE_BATTLE) then
							destroyCreature(cid)
						end
						destroyModifier(cid,mid)
					end
				end
				createModifier(ch,mod)
				moveCard(ch,ZONE_BATTLE)
			end
			shuffleDeck(owner)
		end
		Abils.TapAbility(id,tapAbility)
	end
}

Cards["Dracobarrier"] = {
	shieldtrigger = 1,

	OnCast = function(id)
		local ch = createChoice("Choose an opponent's creature",1,id,getCardOwner(id),Checks.UntappedInOppBattle)
        if(ch>=0) then
			tapCard(ch)

			if(isCreatureOfRace(ch,"Dragon")==1) then
				Functions.moveTopCardsFromDeck(getCardOwner(id), ZONE_SHIELD, 1)
			end
        end
	end
}

Cards["Laser Whip"] = {
	shieldtrigger = 0,

	OnCast = function(id)
		local ch1 = createChoice("Choose an opponent's creature",1,id,getCardOwner(id),Checks.UntappedInOppBattle)
        if(ch1>=0) then
			tapCard(ch1)
        end
		
		local mod = function(cid,mid)
            Abils.cantBeBlocked(cid)
	        Abils.destroyModAtEOT(cid,mid)
        end

		local ch2 = createChoice("Choose a creature in your battlezone",1,id,getCardOwner(id),Checks.InYourBattle)
		if(ch2>=0) then
            createModifier(ch2,mod)
        end
        Functions.EndSpell(id)
	end
}

Cards["Lunar Charger"] = {
	shieldtrigger = 0,

	OnCast = function(id) --test
		local selected = {}
		local valid = function(cid,sid)
			if(Checks.InYourBattle(cid,sid)==1 and selected[sid]~=true) then
				return 1
			end
			return 0
		end
		for i=1,2 do
			local ch = createChoice("Choose a creature to untap at end of turn",1,id,getCardOwner(id),valid)
			if(ch<0) then
				break
			end
			selected[ch] = true
			local mod = function(cid,mid)
				if(getMessageType()=="pre endturn" and getMessageInt("player")==getCardOwner(cid)) then
					if(getCardZone(cid)==ZONE_BATTLE and isCardTapped(cid)==1) then
						local untap = createChoiceNoCheck("Untap this creature?",2,cid,getCardOwner(cid),Checks.False)
						if(untap==RETURN_BUTTON1) then
							untapCard(cid)
						end
					end
					destroyModifier(cid,mid)
				end
			end
			createModifier(ch,mod)
		end
		Functions.EndSpell(id)
	end,

	HandleMessage = function(id)
		Abils.Charger(id)
	end
}

Cards["Migalo, Vizier of Spycraft"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local turboRush = function(id)
			local attack = function(id)
				local owner = getCardOwner(id)
				local selected = {}
				local valid = function(cid,sid)
					if(Checks.InOppShields(cid,sid)==1 and selected[sid]~=true) then
						return 1
					end
					return 0
				end
				for i=1,2 do
					local ch = createChoice("Choose an opponent's shield to look at",1,id,owner,valid)
					if(ch<0) then
						break
					end
					selected[ch] = true
					unflipCard(ch)
					setCardVisibility(ch,owner,1)
				end
				if(next(selected)~=nil) then
					createChoiceNoCheck("Look at shields",1,id,owner,Checks.False)
					for card in pairs(selected) do
						flipCard(card)
						setCardVisibility(card,owner,0)
					end
				end
			end
			Abils.onAttack(id,attack)
		end
		Abils.TurboRush(id,turboRush)
	end
}

Cards["Misha, Channeler of Suns"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="get creaturecanattackcreature" and getMessageInt("defender")==id and isCreatureOfRace(getMessageInt("attacker"),"Dragon")==1) then
			setMessageInt("canattack",CANATTACK_NO)
		end
	end
}

Cards["Nariel, the Oracle"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getCardZone(id)==ZONE_BATTLE and (getMessageType()=="get creaturecanattackcreature" or getMessageType()=="get creaturecanattackplayers")) then
			local attacker = getMessageInt("attacker")
			if(getCreaturePower(attacker)>=3000) then
				setMessageInt("canattack",CANATTACK_NO)
			end
		end
	end
}

Cards["Sasha, Channeler of Suns"] = {
	shieldtrigger = 0,
	blocker = 1,
	breaker = 2,

	HandleMessage = function(id) --test
		if(getMessageType()=="get creaturecanblock" and getMessageInt("blocker")==id and isCreatureOfRace(getMessageInt("attacker"),"Dragon")==0) then
			setMessageInt("canblock",0)
		elseif(getMessageType()=="get creaturepower" and getMessageInt("creature")==id) then
			local attacker = getAttacker()
			local defender = getDefender()
			if((attacker==id and defender>=0 and isCreatureOfRace(defender,"Dragon")==1) or (attacker>=0 and attacker~=id and isCreatureOfRace(attacker,"Dragon")==1)) then
				setMessageInt("power",getMessageInt("power")+6000)
			end
		end
	end
}

Cards["Sol Galla, Halo Guardian"] = {
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="post cardmove" and getCardZone(id)==ZONE_BATTLE and getMessageInt("to")==ZONE_BATTLE and getCardType(getMessageInt("card"))==TYPE_SPELL) then
			local mod = function(cid,mid)
				Abils.bonusPower(cid,3000)
				Abils.destroyModAtEOT(cid,mid)
			end
			createModifier(id,mod)
		end
	end
}

Cards["Solar Grass"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local turboRush = function(id)
			if(getMessageType()=="post creaturebreakshield" and getMessageInt("creature")==id) then
				local owner = getCardOwner(id)
				local size = getZoneSize(owner,ZONE_BATTLE)
				for i=0,(size-1) do
					local creature = getCardAt(owner,ZONE_BATTLE,i)
					if(getCardType(creature)==TYPE_CREATURE and getCardName(creature)~="Solar Grass") then
						untapCard(creature)
					end
				end
			end
		end
		Abils.TurboRush(id,turboRush)
	end
}

Cards["Thrumiss, Zephyr Guardian"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="post creatureattack" and getCardZone(id)==ZONE_BATTLE and getCardOwner(getMessageInt("attacker"))==getCardOwner(id)) then
			local ch = createChoice("Choose an opponent's creature to tap",1,id,getCardOwner(id),Checks.UntappedInOppBattle)
			if(ch>=0) then
				tapCard(ch)
			end
		end
	end
}

Cards["Aqua Grappler"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local attack = function(id)
			local count = Functions.countTappedCreaturesInBattle(getCardOwner(id))-1
			if(count>0) then
				drawCards(getCardOwner(id),count)
			end
		end
		Abils.onAttack(id,attack)
	end
}

Cards["Candy Cluster"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantBeBlocked(id)
	end
}

Cards["Eureka Charger"] = {
	shieldtrigger = 0,

	OnCast = function(id)
		drawCards(getCardOwner(id),1)
	end,
	
	HandleMessage = function(id)
		Abils.Charger(id)
	end
}

Cards["Grape Globbo"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local summon = function(id)
			local owner = getCardOwner(id)
			local opponent = getOpponent(owner)
			local size = getZoneSize(opponent,ZONE_HAND)
			for i=0,(size-1) do
				local card = getCardAt(opponent,ZONE_HAND,i)
				unflipCard(card)
				setCardVisibility(card,owner,1)
			end
			createChoiceNoCheck("Look at your opponent's hand",1,id,owner,Checks.False)
			for i=0,(size-1) do
				local card = getCardAt(opponent,ZONE_HAND,i)
				flipCard(card)
				setCardVisibility(card,owner,0)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Illusion Fish"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.TurboRush(id,Abils.cantBeBlocked)
	end
}

Cards["Lalicious"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local attack = function(id)
			local owner = getCardOwner(id)
			local opponent = getOpponent(owner)
			local handSize = getZoneSize(opponent,ZONE_HAND)
			for i=0,(handSize-1) do
				local card = getCardAt(opponent,ZONE_HAND,i)
				unflipCard(card)
				setCardVisibility(card,owner,1)
			end
			local deckSize = getZoneSize(opponent,ZONE_DECK)
			local top = -1
			if(deckSize>0) then
				top = getCardAt(opponent,ZONE_DECK,deckSize-1)
				unflipCard(top)
				setCardVisibility(top,owner,1)
			end
			createChoiceNoCheck("Look at your opponent's hand and top deck card",1,id,owner,Checks.False)
			for i=0,(handSize-1) do
				local card = getCardAt(opponent,ZONE_HAND,i)
				flipCard(card)
				setCardVisibility(card,owner,0)
			end
			if(top>=0) then
				flipCard(top)
				setCardVisibility(top,owner,0)
			end
		end
		Abils.onAttack(id,attack)
	end
}

Cards["Marine Scramble"] = {
	shieldtrigger = 0,

	OnCast = function(id)
		local mod = function(cid,mid)
			Abils.cantBeBlocked(cid)
			Abils.destroyModAtEOT(cid,mid)
		end
		local apply = function(cid,sid)
			createModifier(sid,mod)
		end
		Functions.executeForCreaturesInBattle(id,getCardOwner(id),apply)
		Functions.EndSpell(id)
	end
}

Cards["Prowling Elephish"] = {
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Vikorakys"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local turboRush = function(id)
			local attack = function(id)
				local owner = getCardOwner(id)
				openDeck(owner)
				local ch = createChoice("Choose a card in your deck",1,id,owner,Checks.InYourDeck)
				closeDeck(owner)
				if(ch>=0) then
					moveCard(ch,ZONE_HAND)
				end
				shuffleDeck(owner)
			end
			Abils.onAttack(id,attack)
		end
		Abils.TurboRush(id,turboRush)
	end
}

Cards["Wave Lance"] = {
	shieldtrigger = 0,

	OnCast = function(id)
		local ch = createChoice("Choose a creature",0,id,getCardOwner(id),Checks.InBattle)
	    if(ch>=0) then
            moveCard(ch,ZONE_HAND)

			if(isCreatureOfRace(ch, "Dragon")==1) then
				drawCards(getCardOwner(id), 1)
			end
        end
        Functions.EndSpell(id)
	end
}

Cards["Corpse Charger"] = {
	shieldtrigger = 0,

	OnCast = function(id)
		local ch = createChoice("Choose a creature in your graveyard",0,id,getCardOwner(id),Checks.CreatureInYourGraveyard)
        if(ch>=0) then
            moveCard(ch,ZONE_HAND)
        end
        Functions.EndSpell(id)
	end,

	HandleMessage = function(id)
		Abils.Charger(id)
	end
}

Cards["Skeleton Vice"] = {
	shieldtrigger = 0,

	OnCast = function(id) --test
		local opponent = getOpponent(getCardOwner(id))
		discardCardAtRandom(opponent,2)
		Functions.EndSpell(id)
	end
}

Cards["Dimension Splitter"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local owner = getCardOwner(id)
			local hasDragon = false
			local size = getZoneSize(owner,ZONE_GRAVEYARD)
			for i=0,(size-1) do
				if(isCreatureOfRace(getCardAt(owner,ZONE_GRAVEYARD,i),"Dragon")==1) then
					hasDragon = true
					break
				end
			end
			if(hasDragon) then
				local ch = createChoiceNoCheck("Return all Dragons from your graveyard?",2,id,owner,Checks.False)
				if(ch==RETURN_BUTTON1) then
					for i=0,(size-1) do
						local card = getCardAt(owner,ZONE_GRAVEYARD,i)
						if(isCreatureOfRace(card,"Dragon")==1) then
							moveCard(card,ZONE_HAND)
						end
					end
				end
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Gachack, Mechanical Doll"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local turboRush = function(id)
			if(getMessageType()=="post creaturebreakshield" and getMessageInt("creature")==id) then
				local ch = createChoice("Choose a creature to destroy",1,id,getCardOwner(id),Checks.InBattle)
				if(ch>=0) then
					destroyCreature(ch)
				end
			end
		end
		Abils.TurboRush(id,turboRush)
	end
}

Cards["Gigaclaws"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local turboRush = function(id)
			local attack = function(id)
				local opponent = getOpponent(getCardOwner(id))
				local size = getZoneSize(opponent,ZONE_HAND)
				for i=0,(size-1) do
					discardCard(getCardAt(opponent,ZONE_HAND,i))
				end
			end
			Abils.onAttack(id,attack)
		end
		Abils.TurboRush(id,turboRush)
	end
}

Cards["Motorcycle Mutant"] = {
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttack(id)

		if(getMessageType()=="post cardmove") then
			if(getCardOwner(getMessageInt("card"))==getCardOwner(id) and getMessageInt("to")==ZONE_BATTLE) then
				destroyCreature(id)
			end
		end
	end
}

Cards["Necrodragon Galbazeek"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		local func = function(id)
			local ch = createChoice("Choose a shield", 0, id, getCardOwner(id), Checks.InYourShields)
			if(ch>=0) then
				moveCard(ch, ZONE_GRAVEYARD)
			end
		end

		Abils.onAttack(id,func)
	end
}

Cards["Necrodragon Giland"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.destroyAfterBattle(id)
	end
}

Cards["Scream Slicer, Shadow of Fear"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="post cardmove" and getCardZone(id)==ZONE_BATTLE and getMessageInt("to")==ZONE_BATTLE) then
			local summoned = getMessageInt("card")
			if(getCardOwner(summoned)==getCardOwner(id) and getCardType(summoned)==TYPE_CREATURE and (isCreatureOfRace(summoned,"Dragon")==1 or isCreatureOfRace(summoned,"Dragonoid")==1)) then
				local minimum = nil
				local total = getTotalCardCount()
				for card=0,(total-1) do
					if(getCardZone(card)==ZONE_BATTLE and getCardType(card)==TYPE_CREATURE) then
						local power = getCreaturePower(card)
						if(minimum==nil or power<minimum) then
							minimum = power
						end
					end
				end
				local valid = function(cid,sid)
					if(getCardZone(sid)==ZONE_BATTLE and getCardType(sid)==TYPE_CREATURE and getCreaturePower(sid)==minimum) then
						return 1
					end
					return 0
				end
				local ch = createChoice("Choose a creature with the least power",0,id,getCardOwner(id),valid)
				if(ch>=0) then
					destroyCreature(ch)
				end
			end
		end
	end
}

Cards["Tyrant Worm"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="post cardmove") then
			if(getCardOwner(getMessageInt("card"))==getCardOwner(id) and getMessageInt("to")==ZONE_BATTLE) then
				destroyCreature(id)
			end
		end
	end
}

Cards["Bruiser Dragon"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local func = function(id)
			local ch = createChoice("Choose a shield", 0, id, getCardOwner(id), Checks.InYourShields)
			if(ch>=0) then
				moveCard(ch, ZONE_GRAVEYARD)
			end
		end
		Abils.onDestroy(id, func)
	end
}

Cards["Furious Onslaught"] = {
	shieldtrigger = 0,

	OnCast = function(id) --test
		local mod = function(cid,mid)
			Abils.bonusPower(cid,4000)
			Abils.Breaker(cid,2)
			if(getMessageType()=="get creaturerace" and getMessageInt("creature")==cid) then
				local race = getMessageString("race")
				if(string.find(race,"Armored Dragon",1,true)==nil) then
					setMessageString("race",race.."/Armored Dragon")
				end
			end
			Abils.destroyModAtEOT(cid,mid)
		end
		local apply = function(cid,sid)
			if(isCreatureOfRace(sid,"Dragonoid")==1) then
				createModifier(sid,mod)
			end
		end
		Functions.executeForCreaturesInBattle(id,getCardOwner(id),apply)
		Functions.EndSpell(id)
	end
}

Cards["Kyrstron, Lair Delver"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local check = function(cid,sid)
			if(getCardOwner(sid)==getCardOwner(cid) and getCardZone(sid)==ZONE_HAND and isCreatureOfRace(sid, "Dragon")==1) then
				return 1
			else
				return 0
			end
		end
		
		local func = function(id)
			local ch = createChoice("Choose a dragon in your hand", 0, id, getCardOwner(id), check)
			if(ch>=0) then
				moveCard(ch, ZONE_BATTLE)
			end
		end
		Abils.onDestroy(id,func)
	end
}

Cards["Magmadragon Melgars"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Missile Soldier Ultimo"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local turboRush = function(id)
			Abils.canAttackUntappedCreatures(id)
			Abils.PowerAttacker(id,4000)
		end
		Abils.TurboRush(id,turboRush)
	end
}

Cards["Rocketdive Skyterror"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantBeAttacked(id)
		Abils.cantAttackPlayers(id)
		Abils.PowerAttacker(id,1000)
	end
}

Cards["Slaphappy Soldier Galback"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local turboRush = function(id)
			local attack = function(id)
				local valid = function(cid,sid)
					if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=4000) then
						return 1
					end
					return 0
				end
				local ch = createChoice("Choose an opponent's creature with power 4000 or less",1,id,getCardOwner(id),valid)
				if(ch>=0) then
					destroyCreature(ch)
				end
			end
			Abils.onAttack(id,attack)
		end
		Abils.TurboRush(id,turboRush)
	end
}

Cards["Torpedo Skyterror"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local c = Functions.countTappedCreaturesInBattle(getCardOwner(id))-1
		Abils.PowerAttacker(id,c*2000)
	end
}

Cards["Totto Pipicchi"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="get creatureisspeedattacker" and getCardZone(id)==ZONE_BATTLE) then
			local creature = getMessageInt("creature")
			if(getCardZone(creature)==ZONE_BATTLE and isCreatureOfRace(creature,"Dragon")==1) then
				setMessageInt("isspeedattacker",1)
			end
		end
	end
}

Cards["Volcano Charger"] = {
	shieldtrigger = 0,

	OnCast = function(id)
		local valid = function(cid,sid)
            if(getCardOwner(sid)~=getCardOwner(cid) and getCardZone(sid)==ZONE_BATTLE and getCreaturePower(sid)<=2000) then
		        return 1
	        else
		        return 0
	        end
        end
        local ch = createChoice("Choose an opponent's creature",0,id,getCardOwner(id),valid)
	    if(ch>=0) then
            destroyCreature(ch)
        end
        Functions.EndSpell(id)
	end,

	HandleMessage = function(id)
		Abils.Charger(id)
	end
}

Cards["Bakkra Horn, the Silent"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="post cardmove") then
			local summoned = getMessageInt("card")
			if(getCardOwner("summoned")==getCardOwner(id) and getMessageInt("to")==ZONE_BATTLE) then
				if(isCreatureOfRace(summoned, "Dragon")==id or isCreatureOfRace(summoned, "Dragonoid")==id) then
					Functions.moveTopCardsFromDeck(getCardOwner(id), ZONE_MANA, 1)
				end
			end
		end
	end
}

Cards["Carbonite Scarab"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local turboRush = function(id)
			if(getMessageType()=="post creaturebattle" and getMessageInt("blocked")==1 and getMessageInt("attacker")==id) then
				local ch = createChoice("Choose an opponent's shield",0,id,getCardOwner(id),Checks.InOppShields)
				if(ch>=0) then
					creatureBreakShield(id,ch)
				end
			end
		end
		Abils.TurboRush(id,turboRush)
	end
}

Cards["Coliseum Shell"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="post creatureblock" and getMessageInt("attacker")==id) then
			local ch = createChoiceNoCheck("Put the top card of your deck into your mana zone?",2,id,getCardOwner(id),Checks.False)
			if(ch==RETURN_BUTTON1) then
				Functions.moveTopCardsFromDeck(getCardOwner(id),ZONE_MANA,1)
			end
		end
	end
}

Cards["Dracodance Totem"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="mod creaturedestroy" and getMessageInt("creature")==id and getCardZone(id)==ZONE_BATTLE) then
			local valid = function(cid,sid)
				if(Checks.CreatureInYourMana(cid,sid)==1 and isCreatureOfRace(sid,"Dragon")==1) then
					return 1
				end
				return 0
			end
			local ch = createChoice("Choose a Dragon in your mana zone",0,id,getCardOwner(id),valid)
			if(ch>=0) then
				setMessageInt("zoneto",ZONE_MANA)
				moveCard(ch,ZONE_HAND)
			end
		end
	end
}

Cards["Muscle Charger"] = {
	shieldtrigger = 0,

	OnCast = function(id)
		local mod = function(cid,mid)
            Abils.bonusPower(cid,3000)
		    Abils.destroyModAtEOT(cid,mid)
        end
        local func = function(cid,sid)
            createModifier(sid,mod)
        end
		Functions.executeForCreaturesInBattle(id,getCardOwner(id),func)
        Functions.EndSpell(id)
	end,

	HandleMessage = function(id)
		Abils.Charger(id)
	end
}

Cards["Quixotic Hero Swine Snout"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="post cardmove" and getCardZone(id)==ZONE_BATTLE and getMessageInt("to")==ZONE_BATTLE) then
			local summoned = getMessageInt("card")
			if(summoned~=id and getCardType(summoned)==TYPE_CREATURE) then
				local mod = function(cid,mid)
					Abils.bonusPower(cid,3000)
					Abils.destroyModAtEOT(cid,mid)
				end
				createModifier(id,mod)
			end
		end
	end
}

Cards["Root Charger"] = {
	shieldtrigger = 0,

	OnCast = function(id)
		local owner = getCardOwner(id)
		local mod = function(cid,mid)
			if(getMessageType()=="mod creaturedestroy") then
				local creature = getMessageInt("creature")
				if(getCardOwner(creature)==owner and getCardZone(creature)==ZONE_BATTLE) then
					setMessageInt("zoneto",ZONE_MANA)
				end
			end
			Abils.destroyModAtEOT(cid,mid)
		end
		createModifier(id,mod)
	end,

	HandleMessage = function(id)
		Abils.Charger(id)
	end
}

Cards["Senia, Orchard Avenger"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local turboRush = function(id)
			Abils.bonusPower(id,5000)
			Abils.Breaker(id,2)
		end
		Abils.TurboRush(id,turboRush)
	end
}

Cards["Terradragon Gamiratar"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		local check = function(cid,sid)
			if(getCardOwner(sid)~=getCardOwner(cid) and getCardZone(sid)==ZONE_HAND and getCardType(sid)==TYPE_CREATURE) then
				return 1
			else
				return 0
			end
		end

		local func = function(id)
			local ch = createChoice("Choose a creature in your hand", 1, id, getOpponent(getCardOwner(id)), check)
			if(ch>=0) then
				moveCard(ch, ZONE_BATTLE)
			end
		end

		Abils.onSummon(id, func)
	end
}

Cards["Terradragon Regarion"] = {
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.PowerAttacker(id,3000)
	end
}
