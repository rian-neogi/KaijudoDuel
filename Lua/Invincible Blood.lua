package.path = package.path .. ';./?.lua;'
require("Lua/Invincible Legend")

local chooseRace = function(id,player,check,prompt,buttons)
	local ch = createChoice(prompt or "Choose a creature to choose its race",buttons or 0,id,player,check)
	if(ch>=0) then
		return getCreatureRace(ch)
	end
	return nil
end

local onUnblockedPlayerAttack = function(id,func)
	if(getMessageType()=="post creatureunblocked" and getMessageInt("attacker")==id and getMessageInt("defendertype")==DEFENDER_PLAYER) then
		func(id)
	end
end

local topFourToHandAndBottom = function(id)
	local owner = getCardOwner(id)
	local size = getZoneSize(owner,ZONE_DECK)
	local count = math.min(4,size)
	local cards = {}
	for i=1,count do
		local card = getCardAt(owner,ZONE_DECK,size-i)
		cards[card] = true
		unflipCard(card)
		setCardVisibility(card,owner,1)
	end
	local valid = function(cid,sid)
		if(cards[sid]==true) then return 1 end
		return 0
	end
	local chosen = createChoice("Choose a card to put into your hand",0,id,owner,valid)
	if(chosen>=0) then
		cards[chosen] = nil
		moveCard(chosen,ZONE_HAND)
	end
	local ordered = {}
	local remaining = function(cid,sid)
		if(cards[sid]==true and ordered[sid]~=true) then return 1 end
		return 0
	end
	for i=1,(count-1) do
		local ch = createChoice("Choose the next card for the bottom of your deck",0,id,owner,remaining)
		if(ch>=0) then
			ordered[ch] = true
			moveCard(ch,ZONE_DECK,1)
		end
	end
	for card in pairs(cards) do
		flipCard(card)
		setCardVisibility(card,owner,0)
	end
end

local cantBeBlockedWhileAttackingCreature = function(id)
	if(getMessageType()=="get creaturecanblock" and getMessageInt("attacker")==id and getDefenderType()==DEFENDER_CREATURE) then
		setMessageInt("canblock",0)
	end
end

Cards["Petrova, Channeler of Suns"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local summon = function(id)
			local valid = function(cid,sid)
				if(Checks.InBattle(cid,sid)==1 and isCreatureOfRace(sid,"Mecha Del Sol")==0) then return 1 end
				return 0
			end
			local race = chooseRace(id,getCardOwner(id),valid,"Choose a race other than Mecha Del Sol",0)
			if(race~=nil) then
				local mod = function(cid,mid)
					if(getCardZone(cid)==ZONE_BATTLE and getMessageType()=="get creaturepower") then
						local creature = getMessageInt("creature")
						if(getCardZone(creature)==ZONE_BATTLE and isCreatureOfRace(creature,race)==1) then
							setMessageInt("power",getMessageInt("power")+4000)
						end
					end
				end
				createModifier(id,mod)
			end
		end
		Abils.onSummon(id,summon)
		if(getMessageType()=="get creaturecanbechosen" and getMessageInt("creature")==id and getCardZone(id)==ZONE_BATTLE) then
			if(getMessageInt("chooser")~=getCardOwner(id)) then setMessageInt("canchoose",0) end
		end
	end
}

Cards["Aqua Master"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		onUnblockedPlayerAttack(id,function(id)
			local owner = getCardOwner(id)
			local ch = createChoice("Choose an opponent's shield to turn face up",0,id,owner,Checks.InOppShields)
			if(ch>=0) then
				unflipCard(ch)
				setCardVisibility(ch,0,1)
				setCardVisibility(ch,1,1)
			end
		end)
	end
}

Cards["Stallob, the Lifequasher"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		local death = function(id)
			local func = function(cid,sid)
				destroyCreature(sid)
			end
			Functions.executeForCreaturesInBattle(id, getCardOwner(id), func)
			Functions.executeForCreaturesInBattle(id, getOpponent(getCardOwner(id)), func)
		end
		Abils.onDestroy(id,death)
	end
}

Cards["Magmadragon Ogrist Vhal"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="get creaturepower" and getMessageInt("creature")==id) then
			setMessageInt("power",getMessageInt("power")+3000*getZoneSize(getCardOwner(id),ZONE_HAND))
		elseif(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id) then
			local power = getCreaturePower(id)
			if(power>=15000) then
				setMessageInt("breaker",3)
			elseif(power>=6000 and getMessageInt("breaker")<2) then
				setMessageInt("breaker",2)
			end
		end
	end
}

Cards["Stratosphere Giant"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 3,

	HandleMessage = function(id) --test
		local check = function(cid,sid)
			if(getCardOwner(sid)~=getCardOwner(cid) and getCardZone(sid)==ZONE_HAND and getCardType(sid)==TYPE_CREATURE) then
				return 1
			else
				return 0
			end
		end

		local func = function(id)
			local ch = createChoice("Choose a creature in your hand", 0, id, getOpponent(getCardOwner(id)), check)
			if(ch>=0) then
				moveCard(ch, ZONE_BATTLE)

				local ch2 = createChoice("Choose a creature in your hand", 0, id, getOpponent(getCardOwner(id)), check)
				if(ch2>=0) then
					moveCard(ch2, ZONE_BATTLE)
				end
			end
		end

		Abils.onSummon(id, func)
	end
}

Cards["Glena Vuele, the Hypnotic"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Evolution(id,"Guardian")
		Abils.Breaker(id,2)
		if(getMessageType()=="post shieldtriggerused" and getCardZone(id)==ZONE_BATTLE) then
			local trigger = getMessageInt("trigger")
			if(getCardOwner(trigger)~=getCardOwner(id)) then
				local ch = createChoiceNoCheck("Add the top card of your deck to your shields?",2,id,getCardOwner(id),Checks.False)
				if(ch==RETURN_BUTTON1) then Functions.moveTopCardsFromDeck(getCardOwner(id),ZONE_SHIELD,1) end
			end
		end
	end
}

Cards["Marching Motherboard"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="post cardmove" and getCardZone(id)==ZONE_BATTLE) then
			local creature = getMessageInt("card")
			if(creature~=id and getMessageInt("to")==ZONE_BATTLE and getCardOwner(creature)==getCardOwner(id)
				and getCardType(creature)==TYPE_CREATURE and string.find(getCreatureRace(creature),"Cyber",1,true)~=nil) then
				local ch = createChoiceNoCheck("Draw a card?",2,id,getCardOwner(id),Checks.False)
				if(ch==RETURN_BUTTON1) then drawCards(getCardOwner(id),1) end
			end
		end
	end
}

Cards["Azaghast, Tyrant of Shadows"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.Evolution(id,"Dark Lord")
		if(getMessageType()=="post cardmove") then
			local cid = getMessageInt("card")
			if(getCardOwner(cid)==getCardOwner(id) and getMessageInt("to")==ZONE_BATTLE and isCreatureOfRace(cid,"Ghost")==1) then
				local ch = createChoice("Choose an opponent's creature",0,id,getCardOwner(id),Checks.UntappedInOppBattle)
				if(ch>=0) then
					destroyCreature(ch)
				end
			end
		end
	end
}

Cards["Balesk Baj, the Timeburner"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Evolution(id,"Armored Wyvern")
		Abils.Breaker(id,2)
		Abils.returnAtEOT(id)
		onUnblockedPlayerAttack(id,function(id)
			local owner = getCardOwner(id)
			local mod = function(cid,mid)
				if(getMessageType()=="mod endturn" and getMessageInt("player")==owner) then
					setMessageInt("extraturn",1)
					destroyModifier(cid,mid)
				end
			end
			createModifier(id,mod)
		end)
	end
}

Cards["Vreemah, Freaky Mojo Totem"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="post cardmove" and getCardZone(id)==ZONE_BATTLE) then
			local summoned = getMessageInt("card")
			if(summoned~=id and getMessageInt("to")==ZONE_BATTLE and getCardOwner(summoned)==getCardOwner(id)
				and getCardType(summoned)==TYPE_CREATURE) then
				local mod = function(cid,mid)
					Abils.bonusPower(cid,2000)
					Abils.Breaker(cid,2)
					Abils.destroyModAtEOT(cid,mid)
				end
				for player=0,1 do
					local size = getZoneSize(player,ZONE_BATTLE)
					for i=0,(size-1) do
						local creature = getCardAt(player,ZONE_BATTLE,i)
						if(isCreatureOfRace(creature,"Beast Folk")==1) then createModifier(creature,mod) end
					end
				end
			end
		end
	end
}

Cards["Betrale, the Explorer"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.untapAtEOT(id)
		Abils.cantAttackPlayers(id)
	end
}

Cards["Cosmic Wing"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id)
		local mod = function(cid,mid)
            Abils.cantBeBlocked(cid)
	        Abils.destroyModAtEOT(cid,mid)
        end
		local ch = createChoice("Choose creature",0,id,getCardOwner(id),Checks.InYourBattle)
		if(ch>=0) then
            createModifier(ch,mod)
        end
        Functions.EndSpell(id)
	end
}

Cards["Cyclolink, Spectral Knight"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		onUnblockedPlayerAttack(id,function(id)
			local owner = getCardOwner(id)
			openDeck(owner)
			local ch = createChoice("Choose a spell in your deck",1,id,owner,Checks.SpellInYourDeck)
			if(ch>=0) then moveCard(ch,ZONE_HAND) end
			shuffleDeck(owner)
			closeDeck(owner)
		end)
	end
}

Cards["Jil Warka, Time Guardian"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.cantAttackPlayers(id)
		local death = function(id)
			local selected = {}
			local valid = function(cid,sid)
				if(Checks.UntappedInOppBattle(cid,sid)==1 and selected[sid]~=true) then return 1 end
				return 0
			end
			for i=1,2 do
				local ch = createChoice("Choose an opponent's creature to tap",1,id,getCardOwner(id),valid)
				if(ch<0) then break end
				selected[ch] = true
				tapCard(ch)
			end
		end
		Abils.onDestroy(id,death)
	end
}

Cards["Kalute, Vizier of Eternity"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="mod creaturedestroy" and getMessageInt("creature")==id and getCardZone(id)==ZONE_BATTLE) then
			local owner = getCardOwner(id)
			local size = getZoneSize(owner,ZONE_BATTLE)
			for i=0,(size-1) do
				local creature = getCardAt(owner,ZONE_BATTLE,i)
				if(creature~=id and getCardName(creature)=="Kalute, Vizier of Eternity") then
					setMessageInt("zoneto",ZONE_HAND)
					break
				end
			end
		end
	end
}

Cards["Micute, the Oracle"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="post cardmove" and getCardZone(id)==ZONE_BATTLE) then
			local creature = getMessageInt("card")
			if(getMessageInt("to")==ZONE_BATTLE and getCardOwner(creature)==getCardOwner(id) and isCreatureOfRace(creature,"Guardian")==1) then
				local ch = createChoice("Choose an opponent's creature to tap",1,id,getCardOwner(id),Checks.UntappedInOppBattle)
				if(ch>=0) then tapCard(ch) end
			end
		end
	end
}

Cards["Mihail, Celestial Elemental"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="mod creaturedestroy" and getCardZone(id)==ZONE_BATTLE) then
			local creature = getMessageInt("creature")
			if(creature~=id and getCardZone(creature)==ZONE_BATTLE) then setMessageInt("msgContinue",0) end
		end
	end
}

Cards["Nexus Charger"] = {
	price_tier = 3,
	shieldtrigger = 0,

	OnCast = function(id)
		local ch = createChoice("Choose a card in your hand",0,id,getCardOwner(id),Checks.InYourHand)
		if(ch>=0) then moveCard(ch,ZONE_SHIELD) end
		Functions.EndSpell(id)
	end,

	HandleMessage = function(id)
		Abils.Charger(id)
	end
}

Cards["Tra Rion, Penumbra Guardian"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local tap = function(id)
			local race = chooseRace(id,getCardOwner(id),Checks.InBattle,"Choose a creature to choose its race",0)
			if(race~=nil) then
				local mod = function(cid,mid)
					if(getMessageType()=="pre endturn") then
						for player=0,1 do
							local size = getZoneSize(player,ZONE_BATTLE)
							for i=0,(size-1) do
								local creature = getCardAt(player,ZONE_BATTLE,i)
								if(isCreatureOfRace(creature,race)==1) then untapCard(creature) end
							end
						end
						destroyModifier(cid,mid)
					end
				end
				createModifier(id,mod)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Unified Resistance"] = {
	price_tier = 2,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local owner = getCardOwner(id)
		local race = chooseRace(id,owner,Checks.InYourBattle,"Choose one of your creatures to choose its race",0)
		if(race~=nil) then
			local size = getZoneSize(owner,ZONE_BATTLE)
			for i=0,(size-1) do
				local creature = getCardAt(owner,ZONE_BATTLE,i)
				if(isCreatureOfRace(creature,race)==1) then
					local mod = function(cid,mid)
						Abils.Blocker(cid)
						if(getMessageType()=="pre startturn" and getMessageInt("player")==owner) then destroyModifier(cid,mid) end
					end
					createModifier(creature,mod)
				end
			end
		end
		Functions.EndSpell(id)
	end
}

Cards["Abduction Charger"] = {
	price_tier = 3,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local selected = {}
		local valid = function(cid,sid)
			if(Checks.InBattle(cid,sid)==1 and selected[sid]~=true) then return 1 end
			return 0
		end
		for i=1,2 do
			local ch = createChoice("Choose a creature to return to its owner's hand",1,id,getCardOwner(id),valid)
			if(ch<0) then break end
			selected[ch] = true
			moveCard(ch,ZONE_HAND)
		end
		Functions.EndSpell(id)
	end,

	HandleMessage = function(id)
		Abils.Charger(id)
	end
}

Cards["Emperor Maroll"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Evolution(id,"Cyber Lord")
		if(getMessageType()=="post cardmove" and getCardZone(id)==ZONE_BATTLE) then
			local summoned = getMessageInt("card")
			if(summoned~=id and getMessageInt("to")==ZONE_BATTLE and getCardOwner(summoned)==getCardOwner(id)
				and getCardType(summoned)==TYPE_CREATURE) then moveCard(id,ZONE_HAND) end
		elseif(getMessageType()=="mod creaturebattle" and getMessageInt("blocked")==1 and getMessageInt("attacker")==id) then
			setMessageInt("msgContinue",0)
			moveCard(getMessageInt("defender"),ZONE_HAND)
		end
	end
}

Cards["Hokira"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local tap = function(id)
			local owner = getCardOwner(id)
			local race = chooseRace(id,owner,Checks.InYourBattle,"Choose one of your creatures to choose its race",0)
			if(race~=nil) then
				local mod = function(cid,mid)
					if(getMessageType()=="mod creaturedestroy") then
						local creature = getMessageInt("creature")
						if(getCardOwner(creature)==owner and isCreatureOfRace(creature,race)==1) then setMessageInt("zoneto",ZONE_HAND) end
					end
					Abils.destroyModAtEOT(cid,mid)
				end
				createModifier(id,mod)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Impossible Tunnel"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local race = chooseRace(id,getCardOwner(id),Checks.InBattle,"Choose a creature to choose its race",0)
		if(race~=nil) then
			local mod = function(cid,mid)
				if(getMessageType()=="get creaturecanblock" and isCreatureOfRace(getMessageInt("attacker"),race)==1) then
					setMessageInt("canblock",0)
				end
				Abils.destroyModAtEOT(cid,mid)
			end
			createModifier(id,mod)
		end
		Functions.EndSpell(id)
	end
}

Cards["Kelp Candle"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.cantAttack(id)
		if(getMessageType()=="post creaturebattle" and getMessageInt("blocked")==1 and getMessageInt("defender")==id) then
			topFourToHandAndBottom(id)
		end
	end
}

Cards["Scout Cluster"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="post cardmove" and getCardZone(id)==ZONE_BATTLE) then
			local summoned = getMessageInt("card")
			if(summoned~=id and getMessageInt("to")==ZONE_BATTLE and getCardOwner(summoned)==getCardOwner(id)
				and getCardType(summoned)==TYPE_CREATURE) then moveCard(id,ZONE_HAND) end
		end
	end
}

Cards["Submarine Project"] = {
	price_tier = 2,
	shieldtrigger = 1,

	OnCast = function(id) --test
		topFourToHandAndBottom(id)
		Functions.EndSpell(id)
	end
}

Cards["Tekorax"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local summon = function(id)
			local owner = getCardOwner(id)
			local opponent = getOpponent(owner)
			local shields = {}
			local size = getZoneSize(opponent,ZONE_SHIELD)
			for i=0,(size-1) do
				local card = getCardAt(opponent,ZONE_SHIELD,i)
				shields[#shields+1] = card
				unflipCard(card)
				setCardVisibility(card,owner,1)
			end
			createChoiceNoCheck("Look at your opponent's shields",1,id,owner,Checks.False)
			for _,card in ipairs(shields) do
				flipCard(card)
				setCardVisibility(card,owner,0)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Tentacle Cluster"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		onUnblockedPlayerAttack(id,function(id)
			local ch = createChoice("Choose a creature to return to its owner's hand",1,id,getCardOwner(id),Checks.InBattle)
			if(ch>=0) then moveCard(ch,ZONE_HAND) end
		end)
	end
}

Cards["Zeppelin Crawler"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.cantAttack(id)
		if(getMessageType()=="post creaturebattle" and getMessageInt("blocked")==1 and getMessageInt("defender")==id) then
			topFourToHandAndBottom(id)
		end
	end
}

Cards["Acid Reflux, the Fleshboiler"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttack(id)
		Abils.Slayer(id)
	end
}

Cards["Bat Doctor, Shadow of Undeath"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local death = function(id)
			local ch = createChoice("Choose another creature in your graveyard",1,id,getCardOwner(id),Checks.CreatureInYourGraveyard)
			if(ch>=0) then moveCard(ch,ZONE_HAND) end
		end
		Abils.onDestroy(id,death)
	end
}

Cards["Gabzagul, Warlord of Pain"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE) then
			for player=0,1 do
				local size = getZoneSize(player,ZONE_BATTLE)
				for i=0,(size-1) do Abils.attacksEachTurn(getCardAt(player,ZONE_BATTLE,i)) end
			end
		end
	end
}

Cards["Grinning Hunger"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id)
		local opponent = getOpponent(getCardOwner(id))
		local valid = function(cid,sid)
			if(getCardOwner(sid)==opponent and (getCardZone(sid)==ZONE_BATTLE or getCardZone(sid)==ZONE_SHIELD)) then return 1 end
			return 0
		end
		local ch = createChoice("Choose one of your creatures or shields to put into your graveyard",0,id,opponent,valid)
		if(ch>=0) then
			if(getCardZone(ch)==ZONE_BATTLE) then destroyCreature(ch) else moveCard(ch,ZONE_GRAVEYARD) end
		end
		Functions.EndSpell(id)
	end
}

Cards["Ice Vapor, Shadow of Anguish"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="post cardmove" and getCardZone(id)==ZONE_BATTLE) then
			local spell = getMessageInt("card")
			local owner = getCardOwner(id)
			local opponent = getOpponent(owner)
			if(getMessageInt("to")==ZONE_BATTLE and getCardType(spell)==TYPE_SPELL and getCardOwner(spell)==opponent) then
				local handCheck = function(cid,sid)
					if(getCardOwner(sid)==opponent and getCardZone(sid)==ZONE_HAND) then return 1 end
					return 0
				end
				local discard = createChoice("Choose a card from your hand to discard",0,id,opponent,handCheck)
				if(discard>=0) then discardCard(discard) end
				local manaCheck = function(cid,sid)
					if(getCardOwner(sid)==opponent and getCardZone(sid)==ZONE_MANA) then return 1 end
					return 0
				end
				local mana = createChoice("Choose a card in your mana zone to put into your graveyard",0,id,opponent,manaCheck)
				if(mana>=0) then destroyMana(mana) end
			end
		end
	end
}

Cards["Necrodragon Izorist Vhal"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="get creaturepower" and getMessageInt("creature")==id) then
			local owner = getCardOwner(id)
			local count = 0
			local size = getZoneSize(owner,ZONE_GRAVEYARD)
			for i=0,(size-1) do
				local card = getCardAt(owner,ZONE_GRAVEYARD,i)
				if(getCardType(card)==TYPE_CREATURE and getCardCiv(card)==CIV_DARKNESS) then count=count+1 end
			end
			setMessageInt("power",getMessageInt("power")+count*2000)
		elseif(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id and getCreaturePower(id)>=6000) then
			if(getMessageInt("breaker")<2) then setMessageInt("breaker",2) end
		end
	end
}

Cards["Slash Charger"] = {
	price_tier = 3,
	shieldtrigger = 0,

	OnCast = function(id)
		local owner = getCardOwner(id)
		local choice = createChoiceNoCheck("Search opponent's deck? (Choose No for your own deck)",2,id,owner,Checks.False,RETURN_BUTTON1)
		local player = owner
		if(choice==RETURN_BUTTON1) then player=getOpponent(owner) end
		local valid = function(cid,sid)
			if(getCardOwner(sid)==player and getCardZone(sid)==ZONE_DECK) then return 1 end
			return 0
		end
		openDeck(player)
		local preferred = RETURN_BUTTON1
		if(getZoneSize(player,ZONE_DECK)>0) then preferred=getCardAt(player,ZONE_DECK,0) end
		local ch = createChoice("Choose a card to put into that player's graveyard",1,id,owner,valid,preferred)
		if(ch>=0) then moveCard(ch,ZONE_GRAVEYARD) end
		shuffleDeck(player)
		closeDeck(player)
		Functions.EndSpell(id)
	end,

	HandleMessage = function(id)
		Abils.Charger(id)
	end
}

Cards["Trixo, Wicked Doll"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		onUnblockedPlayerAttack(id,function(id)
			local opponent = getOpponent(getCardOwner(id))
			local valid = function(cid,sid)
				if(getCardOwner(sid)==opponent and getCardZone(sid)==ZONE_BATTLE and getCardType(sid)==TYPE_CREATURE) then return 1 end
				return 0
			end
			local ch = createChoice("Choose one of your creatures to destroy",0,id,opponent,valid)
			if(ch>=0) then destroyCreature(ch) end
		end)
	end
}

Cards["Venom Worm"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local tap = function(id)
			local race = chooseRace(id,getCardOwner(id),Checks.InBattle,"Choose a creature to choose its race",0)
			if(race~=nil) then
				local mod = function(cid,mid)
					if(getMessageType()=="post creaturebattle") then
						local attacker = getMessageInt("attacker")
						local defender = getMessageInt("defender")
						if(isCreatureOfRace(attacker,race)==1 and getCardZone(defender)==ZONE_BATTLE) then destroyCreature(defender) end
						if(isCreatureOfRace(defender,race)==1 and getCardZone(attacker)==ZONE_BATTLE) then destroyCreature(attacker) end
					end
					Abils.destroyModAtEOT(cid,mid)
				end
				createModifier(id,mod)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Zombie Carnival"] = {
	price_tier = 1,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local owner = getCardOwner(id)
		local race = chooseRace(id,owner,Checks.CreatureInYourGraveyard,"Choose a creature in your graveyard to choose its race",0)
		if(race~=nil) then
			local selected = {}
			local valid = function(cid,sid)
				if(Checks.CreatureInYourGraveyard(cid,sid)==1 and isCreatureOfRace(sid,race)==1 and selected[sid]~=true) then return 1 end
				return 0
			end
			for i=1,3 do
				local ch = createChoice("Choose a creature to return to your hand",1,id,owner,valid)
				if(ch<0) then break end
				selected[ch] = true
				moveCard(ch,ZONE_HAND)
			end
		end
		Functions.EndSpell(id)
	end
}

Cards["Aerodactyl Kooza"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		cantBeBlockedWhileAttackingCreature(id)
		Abils.PowerAttacker(id,3000)
	end
}

Cards["Blizzard of Spears"] = {
	price_tier = 3,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local targets = {}
		for player=0,1 do
			local size = getZoneSize(player,ZONE_BATTLE)
			for i=0,(size-1) do
				local creature = getCardAt(player,ZONE_BATTLE,i)
				if(getCreaturePower(creature)<=4000) then targets[#targets+1]=creature end
			end
		end
		for _,creature in ipairs(targets) do destroyCreature(creature) end
		Functions.EndSpell(id)
	end
}

Cards["Fists of Forever"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local ch = createChoice("Choose one of your creatures",0,id,getCardOwner(id),Checks.InYourBattle)
		if(ch>=0) then
			local mod = function(cid,mid)
				if(getMessageType()=="post creaturebattle" and (getMessageInt("attacker")==cid or getMessageInt("defender")==cid)) then
					local other = getMessageInt("attacker")
					if(other==cid) then other=getMessageInt("defender") end
					if(getCreaturePower(cid)>getCreaturePower(other) and getCardZone(cid)==ZONE_BATTLE) then untapCard(cid) end
				end
				Abils.destroyModAtEOT(cid,mid)
			end
			createModifier(ch,mod)
		end
		Functions.EndSpell(id)
	end
}

Cards["Gigio's Hammer"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local tap = function(id)
			local race = chooseRace(id,getCardOwner(id),Checks.InBattle,"Choose a creature to choose its race",0)
			if(race~=nil) then
				local mod = function(cid,mid)
					local creature = getMessageInt("creature")
					if(getMessageType()=="get creaturepower" and getAttacker()==creature and isCreatureOfRace(creature,race)==1) then
						setMessageInt("power",getMessageInt("power")+4000)
					elseif(getMessageType()=="get creaturemustattack" and isCreatureOfRace(creature,race)==1) then
						setMessageInt("mustattack",1)
					end
					Abils.destroyModAtEOT(cid,mid)
				end
				createModifier(id,mod)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Quakesaur"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		onUnblockedPlayerAttack(id,function(id)
			local opponent = getOpponent(getCardOwner(id))
			local valid = function(cid,sid)
				if(getCardOwner(sid)==opponent and getCardZone(sid)==ZONE_MANA) then return 1 end
				return 0
			end
			local ch = createChoice("Choose a card in your mana zone to put into your graveyard",0,id,opponent,valid)
			if(ch>=0) then destroyMana(ch) end
		end)
	end
}

Cards["Relentless Blitz"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local race = chooseRace(id,getCardOwner(id),Checks.InBattle,"Choose a creature to choose its race",0)
		if(race~=nil) then
			local mod = function(cid,mid)
				if(getMessageType()=="get creaturecanattackcreature" and isCreatureOfRace(getMessageInt("attacker"),race)==1) then
					setMessageInt("canattack",CANATTACK_UNTAPPED)
				elseif(getMessageType()=="get creaturecanblock" and getDefenderType()==DEFENDER_CREATURE
					and isCreatureOfRace(getMessageInt("attacker"),race)==1) then
					setMessageInt("canblock",0)
				end
				Abils.destroyModAtEOT(cid,mid)
			end
			createModifier(id,mod)
		end
		Functions.EndSpell(id)
	end
}

Cards["Shock Trooper Mykee"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.SpeedAttacker(id)
		onUnblockedPlayerAttack(id,function(id)
			local valid = function(cid,sid)
				if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=3000) then return 1 end
				return 0
			end
			local ch = createChoice("Choose an opponent's creature to destroy",1,id,getCardOwner(id),valid)
			if(ch>=0) then destroyCreature(ch) end
		end)
	end
}

Cards["Simian Warrior Grash"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="post creaturedestroy" and getCardZone(id)==ZONE_BATTLE) then
			local creature = getMessageInt("creature")
			if(getCardOwner(creature)==getCardOwner(id) and isCreatureOfRace(creature,"Armorloid")==1) then
				local opponent = getOpponent(getCardOwner(id))
				local valid = function(cid,sid)
					if(getCardOwner(sid)==opponent and getCardZone(sid)==ZONE_MANA) then return 1 end
					return 0
				end
				local ch = createChoice("Choose a card in your mana zone to put into your graveyard",0,id,opponent,valid)
				if(ch>=0) then destroyMana(ch) end
			end
		end
	end
}

Cards["Snaptongue Lizard"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		cantBeBlockedWhileAttackingCreature(id)
		Abils.PowerAttacker(id,3000)
	end
}

Cards["Steam Rumbler Kain"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local attack = function(id)
			local ch = createChoice("Choose one of your shields to put into your graveyard",0,id,getCardOwner(id),Checks.InYourShields)
			if(ch>=0) then moveCard(ch,ZONE_GRAVEYARD) end
		end
		Abils.onAttack(id,attack)
	end
}

Cards["Cavern Raider"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		onUnblockedPlayerAttack(id,function(id)
			local owner = getCardOwner(id)
			openDeck(owner)
			local ch = createChoice("Choose a creature in your deck",1,id,owner,Checks.CreatureInYourDeck)
			if(ch>=0) then moveCard(ch,ZONE_HAND) end
			shuffleDeck(owner)
			closeDeck(owner)
		end)
	end
}

Cards["Dance of the Sproutlings"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local owner = getCardOwner(id)
		local race = chooseRace(id,owner,Checks.InYourHand,"Choose a creature in your hand to choose its race",1)
		if(race~=nil) then
			local selected = {}
			local valid = function(cid,sid)
				if(Checks.InYourHand(cid,sid)==1 and getCardType(sid)==TYPE_CREATURE
					and isCreatureOfRace(sid,race)==1 and selected[sid]~=true) then return 1 end
				return 0
			end
			while(true) do
				local ch = createChoice("Choose a creature to put into your mana zone",1,id,owner,valid)
				if(ch<0) then break end
				selected[ch] = true
				moveCard(ch,ZONE_MANA)
			end
		end
		Functions.EndSpell(id)
	end
}

Cards["Mana Bonanza"] = {
	price_tier = 3,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local owner = getCardOwner(id)
		local count = getZoneSize(owner,ZONE_MANA)
		local deckSize = getZoneSize(owner,ZONE_DECK)
		count = math.min(count,deckSize)
		for i=1,count do
			local card = getCardAt(owner,ZONE_DECK,deckSize-i)
			moveCard(card,ZONE_MANA)
			tapCard(card)
		end
		Functions.EndSpell(id)
	end
}

Cards["Silvermoon Trailblazer"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local tap = function(id)
			local race = chooseRace(id,getCardOwner(id),Checks.InBattle,"Choose a creature to choose its race",0)
			if(race~=nil) then
				local mod = function(cid,mid)
					if(getMessageType()=="get creaturecanblock") then
						local attacker = getMessageInt("attacker")
						local blocker = getMessageInt("blocker")
						if(isCreatureOfRace(attacker,race)==1 and getCreaturePower(blocker)<=3000) then setMessageInt("canblock",0) end
					end
					Abils.destroyModAtEOT(cid,mid)
				end
				createModifier(id,mod)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Solid Horn"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.manaAfterDestroyed(id)
	end
}

Cards["Storm Wrangler, the Furious"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Evolution(id,"Beast Folk")
		local attack = function(id)
			local valid = function(cid,sid)
				if(Checks.BlockerInOppBattle(cid,sid)==1 and isCardTapped(sid)==0) then return 1 end
				return 0
			end
			local ch = createChoice("Choose an opponent's untapped blocker",1,id,getCardOwner(id),valid)
			if(ch>=0) then
				local chosen = ch
				local mod = function(cid,mid)
					if(getMessageType()=="get creaturecanblock" and getMessageInt("attacker")==cid and getMessageInt("blocker")~=chosen) then
						setMessageInt("canblock",0)
					elseif(getMessageType()=="get creatureforcedblocker" and getMessageInt("attacker")==cid
						and getCardZone(chosen)==ZONE_BATTLE and isCardTapped(chosen)==0) then
						setMessageInt("forcedblocker",chosen)
					elseif(getMessageType()=="post resetattack") then
						destroyModifier(cid,mid)
					end
				end
				createModifier(id,mod)
			end
		end
		Abils.onAttack(id,attack)
		local blocksThisTurn = getDuelStateInt("storm_wrangler.blocks_this_turn",id,0)
		if(getMessageType()=="mod creaturebattle" and getMessageInt("blocked")==1 and getMessageInt("attacker")==id) then
			setDuelStateInt("storm_wrangler.blocks_this_turn",id,blocksThisTurn+1)
		elseif(getMessageType()=="get creaturepower" and getMessageInt("creature")==id and blocksThisTurn>0) then
			setMessageInt("power",getMessageInt("power")+3000*blocksThisTurn)
		elseif(getMessageType()=="pre endturn") then
			clearDuelState("storm_wrangler.blocks_this_turn",id)
		end
	end
}

Cards["Terradragon Anrist Vhal"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="get creaturepower" and getMessageInt("creature")==id) then
			local owner = getCardOwner(id)
			local count = 0
			local size = getZoneSize(owner,ZONE_BATTLE)
			for i=0,(size-1) do
				local creature = getCardAt(owner,ZONE_BATTLE,i)
				if(creature~=id and getCardCiv(creature)==CIV_NATURE) then count=count+1 end
			end
			setMessageInt("power",getMessageInt("power")+count*2000)
		elseif(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id and getCreaturePower(id)>=6000) then
			if(getMessageInt("breaker")<2) then setMessageInt("breaker",2) end
		end
	end
}

Cards["Vine Charger"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id)
		local opponent = getOpponent(getCardOwner(id))
		local valid = function(cid,sid)
			if(getCardOwner(sid)==opponent and getCardZone(sid)==ZONE_BATTLE and getCardType(sid)==TYPE_CREATURE) then return 1 end
			return 0
		end
		local ch = createChoice("Choose one of your creatures to put into your mana zone",0,id,opponent,valid)
		if(ch>=0) then moveCard(ch,ZONE_MANA) end
		Functions.EndSpell(id)
	end,

	HandleMessage = function(id)
		Abils.Charger(id)
	end
}

Cards["Whip Scorpion"] = {
	price_tier = 1,
	shieldtrigger = 1,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.PowerAttacker(id,3000)
	end
}

Cards["Whispering Totem"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local owner = getCardOwner(id)
			local valid = function(cid,sid)
				if(Checks.CreatureInYourDeck(cid,sid)==1 and getCardName(sid)=="Whispering Totem") then return 1 end
				return 0
			end
			openDeck(owner)
			local ch = createChoice("Choose a Whispering Totem in your deck",1,id,owner,valid)
			if(ch>=0) then moveCard(ch,ZONE_HAND) end
			shuffleDeck(owner)
			closeDeck(owner)
		end
		Abils.onSummon(id,summon)
	end
}
