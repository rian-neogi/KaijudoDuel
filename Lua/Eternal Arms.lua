package.path = package.path .. ';./?.lua;'
require("Lua/Invincible Blood")

local zoneCards = function(player,zone)
	local cards = {}
	for i=0,getZoneSize(player,zone)-1 do
		cards[#cards+1] = getCardAt(player,zone,i)
	end
	return cards
end

local topCard = function(player)
	local size = getZoneSize(player,ZONE_DECK)
	if(size==0) then
		return -1
	end
	return getCardAt(player,ZONE_DECK,size-1)
end

local moveTop = function(player,zone)
	local card = topCard(player)
	if(card>=0) then
		moveCard(card,zone)
	end
end

local silentNames = {
	["Kejila, the Hidden Horror"] = true,
	["Bulgluf, the Spydroid"] = true,
	["Flohdani, the Spydroid"] = true,
	["Kaemira, the Oracle"] = true,
	["Charge Whipper"] = true,
	["Milporo"] = true,
	["Pinpoint Lunatron"] = true,
	["Gigamente"] = true,
	["Venom Capsule"] = true,
	["Brad, Super Kickin' Dynamo"] = true,
	["Minelord Skyterror"] = true,
	["Vorg's Engine"] = true,
	["Hustle Berry"] = true,
	["Sporeblast Erengi"] = true,
	["Soderlight, the Cold Blade"] = true
}

local silentSkill = function(id,ability)
	if(getMessageType()=="pre startturn" and getMessageInt("player")==getCardOwner(id) and getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==1) then
		local use = createChoiceNoCheck("Use this creature's silent skill?",2,id,getCardOwner(id),Checks.False)
		if(use==RETURN_BUTTON1) then
			local skipUntap = function(cid,mid)
				if(getMessageType()=="mod carduntap" and getMessageInt("card")==cid) then
					setMessageInt("msgContinue",0)
					destroyModifier(cid,mid)
				end
			end
			createModifier(id,skipUntap)
			ability(id)
		end
	end
end

local forcedBlocker = function(id) --test
	if(getMessageType()=="get creatureforcedblocker" and getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==0 and getCardOwner(id)~=getCardOwner(getMessageInt("attacker")) and getMessageInt("forcedblocker")<0) then
		setMessageInt("forcedblocker",id)
	end
end

local destroyByPower = function(id,maximum)
	local cards = zoneCards(0,ZONE_BATTLE)
	for _,card in ipairs(zoneCards(1,ZONE_BATTLE)) do
		cards[#cards+1]=card
	end
	for _,card in ipairs(cards) do
		if(getCreaturePower(card)<=maximum) then
			destroyCreature(card)
		end
	end
end

local chooseShieldToHandWithTrigger = function(id) --test
	local owner = getCardOwner(id)
	local shield = createChoice("Choose one of your shields",0,id,owner,Checks.InYourShields)
	if(shield>=0) then
		moveCard(shield,ZONE_HAND)
		if(getCardIsShieldTrigger(shield)==1) then
			local use = createChoiceNoCheck("Use this shield trigger?",2,id,owner,Checks.False)
			if(use==RETURN_BUTTON1) then
				moveCard(shield,ZONE_BATTLE)
			end
		end
	end
end

local buffUntilEnd = function(source,target,ability)
	local mod = function(cid,mid)
		ability(cid)
		if(getMessageType()=="pre endturn") then
			destroyModifier(cid,mid)
		end
	end
	createModifier(target,mod)
end

local addPowerForTappedMana = function(id,multiplier,player)
	local count = 0
	for _,card in ipairs(zoneCards(player,ZONE_MANA)) do
		if(isCardTapped(card)==1) then
			count=count+1
		end
	end
	Abils.bonusPower(id,count*multiplier)
end

local searchDeck = function(id,check,prompt)
	local owner = getCardOwner(id)
	local card = createChoice(prompt,1,id,owner,check)
	if(card>=0) then
		moveCard(card,ZONE_HAND)
	end
	shuffleDeck(owner)
end

Cards["Deklowaz, the Terminator"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.TapAbility(id,function(id)
			destroyByPower(id,3000)
			local opponent=getOpponent(getCardOwner(id))
			for _,card in ipairs(zoneCards(opponent,ZONE_HAND)) do
				if(getCardType(card)==TYPE_CREATURE and getCreaturePower(card)<=3000) then
					discardCard(card)
				end
			end
		end)
	end
}

Cards["Bluum Erkis, Flare Guardian"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		if(getMessageType()=="mod breakshield" and getMessageInt("attacker")==id) then
			local shield=getMessageInt("shield")
			setMessageInt("msgContinue",0)
			moveCard(shield,ZONE_HAND)
			if(getCardType(shield)==TYPE_SPELL and getCardIsShieldTrigger(shield)==1) then
				moveCard(shield,ZONE_BATTLE)
			end
		end
	end
}

Cards["Hawkeye Lunatron"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		local summon = function(id)
			searchDeck(id,Checks.InYourDeck,"Choose a card from your deck")
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Bodacious Giant"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		local owner=getCardOwner(id)
		if(getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==1 and getTurn()~=owner) then
			for _,card in ipairs(zoneCards(getOpponent(owner),ZONE_BATTLE)) do
				Abils.attacksEachTurn(card)
			end
		end
	end
}

Cards["Terradragon Dakma Balgarow"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.bonusPower(id,(getZoneSize(0,ZONE_SHIELD)+getZoneSize(1,ZONE_SHIELD))*2000)
		if(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id) then
			local power=getCreaturePower(id)
			if(power>=15000) then
				setMessageInt("breaker",3)
			elseif(power>=6000 and getMessageInt("breaker")<2) then
				setMessageInt("breaker",2)
			end
		end
	end
}

Cards["Elixia, Pureblade Elemental"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local seen,count={},0
		for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_MANA)) do
			for civ=CIV_LIGHT,CIV_DARKNESS do
				if(cardHasCivilization(card,civ) and not seen[civ]) then
					seen[civ]=true
					count=count+1
				end
			end
		end
		Abils.bonusPower(id,count*3000)
		if(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id) then
			local power=getCreaturePower(id)
			if(power>=15000) then
				setMessageInt("breaker",3)
			elseif(power>=6000 and getMessageInt("breaker")<2) then
				setMessageInt("breaker",2)
			end
		end
	end
}

Cards["Ultimate Dragon"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local message=getMessageType()
		if((message~="get creaturepower" and message~="get creaturebreaker") or getMessageInt("creature")~=id) then
			return
		end
		local dragons=0
		for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do
			if(card~=id and isCreatureOfRace(card,"Dragon")==1) then
				dragons=dragons+1
			end
		end
		if(message=="get creaturepower") then
			Abils.bonusPower(id,dragons*5000)
		else
			Abils.Breaker(id,dragons+1)
		end
	end
}

Cards["Core-Crash Lizard"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local ch=createChoice("Choose an opponent's shield",0,id,getCardOwner(id),Checks.InOppShields)
			if(ch>=0) then
				moveCard(ch,ZONE_GRAVEYARD)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Hurricane Crawler"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.onSummon(id,function(id)
			local owner=getCardOwner(id)
			local hand=zoneCards(owner,ZONE_HAND)
			for _,card in ipairs(hand) do
				moveCard(card,ZONE_MANA)
			end
			for i=1,#hand do
				local ch=createChoice("Choose a card from your mana zone",0,id,owner,Checks.InYourMana)
				if(ch>=0) then
					moveCard(ch,ZONE_HAND)
				end
			end
		end)
	end
}

Cards["Necrodragon Bryzenaga"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.onSummon(id,function(id)
			local owner=getCardOwner(id)
			for _,shield in ipairs(zoneCards(owner,ZONE_SHIELD)) do
				moveCard(shield,ZONE_HAND)
				if(getCardIsShieldTrigger(shield)==1) then
					local use=createChoiceNoCheck("Use this shield trigger?",2,id,owner,Checks.False)
					if(use==RETURN_BUTTON1) then
						moveCard(shield,ZONE_BATTLE)
					end
				end
			end
		end)
	end
}

Cards["Balza, Seeker of Hyperpearls"] = {
	price_tier = 4,
	shieldtrigger = 1,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Blocker(id) end
}

Cards["Ryudmila, Channeler of Suns"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local count=0
		for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do
			if(card~=id and isCardTapped(card)==0) then
				count=count+1
			end
		end
		Abils.bonusPower(id,count*2000)
		if(getMessageType()=="mod creaturedestroy" and getMessageInt("creature")==id) then
			setMessageInt("zoneto",ZONE_DECK)
			shuffleDeck(getCardOwner(id))
		end
	end
}

Cards["King Oquanos"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		addPowerForTappedMana(id,2000,getOpponent(getCardOwner(id)))
		if(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id and getCreaturePower(id)>=6000) then
			Abils.Breaker(id,2)
		end
	end
}

Cards["Gajirabute, Vile Centurion"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local ch=createChoice("Choose an opponent's shield to put into their graveyard",0,id,getCardOwner(id),Checks.InOppShields)
			if(ch>=0) then
				moveCard(ch,ZONE_GRAVEYARD)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Kejila, the Hidden Horror"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		local ability = function(id)
			for i=1,2 do
				local ch=createChoice("Choose an opponent's shield to break",0,id,getCardOwner(id),Checks.InOppShields)
				if(ch>=0) then
					creatureBreakShield(id,ch)
				end
			end
		end
		silentSkill(id,ability)
	end
}

Cards["Gaulezal Dragon"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
	end
}

Cards["Carnival Totem"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.onSummon(id,function(id)
			local owner=getCardOwner(id)
			local mana=zoneCards(owner,ZONE_MANA)
			local hand=zoneCards(owner,ZONE_HAND)
			for _,card in ipairs(mana) do
				moveCard(card,ZONE_HAND)
			end
			for _,card in ipairs(hand) do
				moveCard(card,ZONE_MANA)
				tapCard(card)
			end
		end)
	end
}

Cards["Tanzanyte, the Awakener"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.TapAbility(id,function(id)
			local chosen=createChoice("Choose a creature in your graveyard",0,id,getCardOwner(id),Checks.CreatureInYourGraveyard)
			if(chosen>=0) then
				local name=getCardName(chosen)
				for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_GRAVEYARD)) do
					if(getCardType(card)==TYPE_CREATURE and getCardName(card)==name) then
						moveCard(card,ZONE_HAND)
					end
				end
			end
		end)
	end
}

Cards["Bombazar, Dragon of Destiny"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.SpeedAttacker(id)
		Abils.onSummon(id,function(id)
			for player=0,1 do
				for _,card in ipairs(zoneCards(player,ZONE_BATTLE)) do
					if(card~=id and getCreaturePower(card)==6000) then
						destroyCreature(card)
					end
				end
			end
			local owner=getCardOwner(id)
			local extra=false
			local mod = function(cid,mid)
				if(getMessageType()=="mod endturn" and getMessageInt("player")==owner) then
					if(not extra) then
						setMessageInt("extraturn",1)
						extra=true
					else
						loseGame(owner)
						destroyModifier(cid,mid)
					end
				end
			end
			createModifier(id,mod)
		end)
	end
}

Cards["Techno Totem"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==1) then
			for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do
				if(card~=id) then
					Abils.PowerAttacker(card,1500)
				end
			end
		end
		local ability = function(id)
			local ch=createChoice("Choose an opponent's creature to tap",0,id,getCardOwner(id),Checks.InOppBattle)
			if(ch>=0) then
				tapCard(ch)
			end
		end
		Abils.TapAbility(id,ability)
	end
}

Cards["Berochika, Channeler of Suns"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local player=getCardOwner(id)
			if(getZoneSize(player,ZONE_SHIELD)>=5) then
				moveTop(player,ZONE_SHIELD)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Bulgluf, the Spydroid"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		silentSkill(id,function(id) moveTop(getCardOwner(id),ZONE_SHIELD) end)
	end
}

Cards["Clearlo, Grace Enforcer"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local count=0
		for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do
			if(card~=id and isCardTapped(card)==0) then
				count=count+1
			end
		end
		Abils.bonusPower(id,count*1000)
	end
}

Cards["Ferrosaturn, Spectral Knight"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Blocker(id)
		Abils.cantAttackPlayers(id)
	end
}

Cards["Flohdani, the Spydroid"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local ability = function(id)
			for i=1,2 do
				local ch=createChoice("Choose an opponent's creature to tap",1,id,getCardOwner(id),Checks.InOppBattle)
				if(ch>=0) then
					tapCard(ch)
				else
					break
				end
			end
		end
		silentSkill(id,ability)
	end
}

Cards["Glais Mejicula, the Extreme"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Evolution(id,"Initiate")
		if(getMessageType()=="mod breakshield" and getCardZone(id)==ZONE_BATTLE and getCardOwner(getMessageInt("shield"))==getCardOwner(id) and getZoneSize(getCardOwner(id),ZONE_HAND)>=2) then
			local use=createChoiceNoCheck("Discard 2 cards instead of breaking this shield?",2,id,getCardOwner(id),Checks.False)
			if(use==RETURN_BUTTON1) then
				for i=1,2 do
					local ch=createChoice("Choose a card to discard",0,id,getCardOwner(id),Checks.InYourHand)
					if(ch>=0) then
						discardCard(ch)
					end
				end
				setMessageInt("msgContinue",0)
			end
		end
	end
}

Cards["Ikaz, the Spydroid"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Blocker(id)
		Abils.cantAttackPlayers(id)
		if(getMessageType()=="post creaturebattle" and getMessageInt("blocked")==1 and getMessageInt("defender")==id) then
			local ch=createChoice("Choose one of your creatures to untap",0,id,getCardOwner(id),Checks.InYourBattle)
			if(ch>=0) then
				untapCard(ch)
			end
		end
	end
}

Cards["Kaemira, the Oracle"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		silentSkill(id,function(id) moveTop(getCardOwner(id),ZONE_SHIELD) end)
	end
}

Cards["Lemik, Vizier of Thought"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getCardZone(id)==ZONE_BATTLE) then
			for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do
				if(cardHasCivilization(card,CIV_WATER) or cardHasCivilization(card,CIV_NATURE)) then
					Abils.Blocker(card)
				end
			end
		end
	end
}

Cards["Logic Cube"] = {
	price_tier = 1,
	shieldtrigger = 1,

	OnCast = function(id)
		searchDeck(id,Checks.SpellInYourDeck,"Choose a spell from your deck")
	end
}

Cards["Messa Bahna, Expanse Guardian"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Blocker(id)
		Abils.cantAttackPlayers(id)
		forcedBlocker(id)
	end
}

Cards["Pala Olesis, Morning Guardian"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Blocker(id)
		Abils.cantAttackPlayers(id)
		if(getCardZone(id)==ZONE_BATTLE and getTurn()~=getCardOwner(id)) then
			for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do
				if(card~=id) then
					Abils.bonusPower(card,2000)
				end
			end
		end
	end
}

Cards["Poltalester, the Spydroid"] = {
	price_tier = 3,
	shieldtrigger = 1,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Blocker(id)
	end
}

Cards["Rapid Reincarnation"] = {
	price_tier = 3,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local owner=getCardOwner(id)
		local victim=createChoice("Choose one of your creatures to destroy",1,id,owner,Checks.InYourBattle)
		if(victim>=0) then
			destroyCreature(victim)
			local valid = function(cid,sid)
				if(Checks.InYourHand(cid,sid)==1 and getCardType(sid)==TYPE_CREATURE and getCardCost(sid)<=getZoneSize(owner,ZONE_MANA)) then
					return 1
				end
				return 0
			end
			local ch=createChoice("Choose a creature to put into battle",0,id,owner,valid)
			if(ch>=0) then
				moveCard(ch,ZONE_BATTLE)
			end
		end
	end
}

Cards["Solar Ray"] = {
	price_tier = 1,
	shieldtrigger = 1,

	OnCast = function(id)
		local ch=createChoice("Choose an opponent's creature to tap",0,id,getCardOwner(id),Checks.InOppBattle)
		if(ch>=0) then
			tapCard(ch)
		end
	end
}

Cards["Static Warp"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local keep={}
		for player=0,1 do
			local valid = function(cid,sid)
				if(Checks.InBattle(cid,sid)==1 and getCardOwner(sid)==player) then
					return 1
				end
				return 0
			end
			local ch=createChoice("Choose a creature to remain untapped",0,id,player,valid)
			if(ch>=0) then
				keep[ch]=true
			end
		end
		for player=0,1 do
			for _,card in ipairs(zoneCards(player,ZONE_BATTLE)) do
				if(not keep[card]) then
					tapCard(card)
				end
			end
		end
	end
}

Cards["Tulk, the Oracle"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Aqua Strummer"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.onSummon(id,function(id)
			local owner=getCardOwner(id)
			local size=getZoneSize(owner,ZONE_DECK)
			local count=math.min(5,size)
			local available={}
			for i=1,count do
				local card=getCardAt(owner,ZONE_DECK,size-i)
				available[card]=true
				unflipCard(card)
				setCardVisibility(card,owner,1)
			end
			local valid = function(cid,sid)
				if(available[sid]) then return 1 end
				return 0
			end
			for i=1,count do
				local ch=createChoice("Choose the next card to put on top",0,id,owner,valid)
				if(ch>=0) then
					available[ch]=nil
					moveCard(ch,ZONE_DECK)
				end
			end
			for card in pairs(available) do
				flipCard(card)
				setCardVisibility(card,owner,0)
			end
		end)
	end
}

Cards["Ardent Lunatron"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Blocker(id)
		Abils.cantAttack(id)
		forcedBlocker(id)
	end
}

Cards["Battery Cluster"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Blocker(id)
		Abils.cantAttack(id)
	end
}

Cards["Buoyant Blowfish"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		addPowerForTappedMana(id,1000,getOpponent(getCardOwner(id)))
	end
}

Cards["Charge Whipper"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local ability = function(id)
			local owner=getCardOwner(id)
			local card=createChoice("Choose a card from your hand to add to shields",1,id,owner,Checks.InYourHand)
			if(card>=0) then
				moveCard(card,ZONE_SHIELD)
				local shield=createChoice("Choose one of your shields to put into your hand",0,id,owner,Checks.InYourShields)
				if(shield>=0) then
					moveCard(shield,ZONE_HAND)
				end
			end
		end
		silentSkill(id,ability)
	end
}

Cards["Crystal Spinslicer"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Blocker(id)
		Abils.Evolution(id,"Liquid People")
	end
}

Cards["Fluorogill Manta"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getCardZone(id)==ZONE_BATTLE) then
			for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do
				if(cardHasCivilization(card,CIV_LIGHT) or cardHasCivilization(card,CIV_DARKNESS)) then
					Abils.cantBeBlocked(card)
				end
			end
		end
	end
}

Cards["Milporo"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		silentSkill(id,function(id) drawCards(getCardOwner(id),1) end)
	end
}

Cards["Mystic Magician"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post cardmove" and getMessageInt("to")==ZONE_BATTLE and silentNames[getCardName(getMessageInt("card"))]) then
			tapCard(getMessageInt("card"))
		end
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="mod creaturedestroy" and getCardOwner(getMessageInt("creature"))==getCardOwner(id) and silentNames[getCardName(getMessageInt("creature"))]) then
			setMessageInt("zoneto",ZONE_HAND)
		end
	end
}

Cards["Pinpoint Lunatron"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local ability = function(id)
			local valid = function(cid,sid)
				if(Checks.InBattle(cid,sid)==1 or getCardZone(sid)==ZONE_MANA) then
					return 1
				end
				return 0
			end
			local ch=createChoice("Choose a creature or mana card to return",0,id,getCardOwner(id),valid)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
		end
		silentSkill(id,ability)
	end
}

Cards["Recon Operation"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local seen={}
		local valid = function(cid,sid)
			if(Checks.InOppShields(cid,sid)==1 and not seen[sid]) then return 1 end
			return 0
		end
		for i=1,3 do
			local ch=createChoice("Choose an opponent's shield to look at",1,id,getCardOwner(id),valid)
			if(ch<0) then break end
			seen[ch]=true
			unflipCard(ch)
			setCardVisibility(ch,getCardOwner(id),1)
		end
		for card in pairs(seen) do
			flipCard(card)
			setCardVisibility(card,getCardOwner(id),0)
		end
	end
}

Cards["Siren Concerto"] = {
	price_tier = 2,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local owner=getCardOwner(id)
		local mana=createChoice("Choose a card from your mana zone",0,id,owner,Checks.InYourMana)
		if(mana>=0) then
			moveCard(mana,ZONE_HAND)
			local hand=createChoice("Choose a card from your hand",0,id,owner,Checks.InYourHand)
			if(hand>=0) then
				moveCard(hand,ZONE_MANA)
			end
		end
	end
}

Cards["Spiral Gate"] = {
	price_tier = 1,
	shieldtrigger = 1,

	OnCast = function(id)
		local ch=createChoice("Choose a creature to return to its owner's hand",0,id,getCardOwner(id),Checks.InBattle)
		if(ch>=0) then
			moveCard(ch,ZONE_HAND)
		end
	end
}

Cards["Tide Patroller"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Blocker(id)
	end
}

Cards["Torpedo Cluster"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local ch=createChoice("Choose a card from your mana zone",0,id,getCardOwner(id),Checks.InYourMana)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Transmogrify"] = {
	price_tier = 3,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local victim=createChoice("Choose a creature to destroy",1,id,getCardOwner(id),Checks.InBattle)
		if(victim>=0) then
			local owner=getCardOwner(victim)
			destroyCreature(victim)
			local deck=zoneCards(owner,ZONE_DECK)
			for i=#deck,1,-1 do
				local card=deck[i]
				if(getCardType(card)==TYPE_CREATURE and getCreatureIsEvolution(card)==0) then
					moveCard(card,ZONE_BATTLE)
					break
				else
					moveCard(card,ZONE_GRAVEYARD)
				end
			end
		end
	end
}

Cards["Zaltan"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post cardmove" and getMessageInt("to")==ZONE_BATTLE and getCardOwner(getMessageInt("card"))==getCardOwner(id) and isCreatureOfRace(getMessageInt("card"),"Cyber Virus")==1) then
			for i=1,2 do
				local discard=createChoice("Choose a card to discard",1,id,getCardOwner(id),Checks.InYourHand)
				if(discard<0) then break end
				discardCard(discard)
				local target=createChoice("Choose a creature to return",0,id,getCardOwner(id),Checks.InBattle)
				if(target>=0) then
					moveCard(target,ZONE_HAND)
				end
			end
		end
	end
}

Cards["Benzo, the Hidden Fury"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.onSummon(id,chooseShieldToHandWithTrigger)
	end
}

Cards["Death Smoke"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id)
		local ch=createChoice("Choose an untapped opponent's creature",0,id,getCardOwner(id),Checks.UntappedInOppBattle)
		if(ch>=0) then
			destroyCreature(ch)
		end
	end
}

Cards["Dedreen, the Hidden Corrupter"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local opponent=getOpponent(getCardOwner(id))
			if(getZoneSize(opponent,ZONE_SHIELD)<=3) then
				discardCardAtRandom(opponent)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Gigamente"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local ability = function(id)
			local ch=createChoice("Choose a creature from your graveyard",0,id,getCardOwner(id),Checks.CreatureInYourGraveyard)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
		end
		silentSkill(id,ability)
	end
}

Cards["Gigandura"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local summon = function(id)
			local card=createChoice("Choose a card in your opponent's hand",1,id,getCardOwner(id),Checks.InOppHand)
			if(card>=0) then
				moveCard(card,ZONE_MANA)
				local mana=createChoice("Choose a card in your opponent's mana zone",0,id,getCardOwner(id),Checks.InOppMana)
				if(mana>=0) then
					moveCard(mana,ZONE_HAND)
				end
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Hourglass Mutant"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getCardZone(id)==ZONE_BATTLE) then
			for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do
				if(cardHasCivilization(card,CIV_WATER) or cardHasCivilization(card,CIV_FIRE)) then
					Abils.Slayer(card)
				end
			end
		end
	end
}

Cards["Infernal Command"] = {
	price_tier = 1,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local ch=createChoice("Choose an opponent's creature",0,id,getCardOwner(id),Checks.InOppBattle)
		if(ch>=0) then
			local owner=getCardOwner(id)
			local mod = function(cid,mid)
				Abils.attacksEachTurn(cid)
				if(getMessageType()=="pre startturn" and getMessageInt("player")==owner) then
					destroyModifier(cid,mid)
				end
			end
			createModifier(ch,mod)
		end
	end
}

Cards["Mikay, Rattling Doll"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Blocker(id)
		Abils.cantAttack(id)
	end
}

Cards["Mummy Wrap, Shadow of Fatigue"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local ability = function(id)
			discardCardAtRandom(0)
			discardCardAtRandom(1)
		end
		Abils.TapAbility(id,ability)
	end
}

Cards["Nightmare Invader"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Pierr, Psycho Doll"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Blocker(id)
		Abils.cantAttack(id)
		Abils.Slayer(id)
		forcedBlocker(id)
	end
}

Cards["Spark Chemist, Shadow of Whim"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_MANA)) do
				moveCard(card,ZONE_HAND)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Spinal Parasite"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="pre startturn" and getMessageInt("player")~=getCardOwner(id)) then
			local player=getMessageInt("player")
			local valid = function(cid,sid)
				if(getCardOwner(sid)==player and Checks.InBattle(cid,sid)==1 and isCardTapped(sid)==0) then
					return 1
				end
				return 0
			end
			local ch=createChoice("Choose a creature that must attack this turn",0,id,player,valid)
			if(ch>=0) then
				local mod = function(cid,mid)
					Abils.attacksEachTurn(cid)
					if(getMessageType()=="pre endturn" and getMessageInt("player")==player) then
						destroyModifier(cid,mid)
					end
				end
				createModifier(ch,mod)
			end
		end
	end
}

Cards["Uliya, the Entrancer"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.onSummon(id,chooseShieldToHandWithTrigger)
	end
}

Cards["Upheaval"] = {
	price_tier = 3,
	shieldtrigger = 1,

	OnCast = function(id) --test
		for player=0,1 do
			local mana=zoneCards(player,ZONE_MANA)
			local hand=zoneCards(player,ZONE_HAND)
			for _,card in ipairs(mana) do
				moveCard(card,ZONE_HAND)
			end
			for _,card in ipairs(hand) do
				moveCard(card,ZONE_MANA)
				tapCard(card)
			end
		end
	end
}

Cards["Venom Capsule"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local ability = function(id)
			local ch=createChoice("Choose an opponent's shield to break",0,id,getCardOwner(id),Checks.InOppShields)
			if(ch>=0) then
				creatureBreakShield(id,ch)
			end
		end
		silentSkill(id,ability)
	end
}

Cards["Zero Nemesis, Shadow of Panic"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id,"Ghost")
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post creatureattack" and getCardOwner(getMessageInt("attacker"))==getCardOwner(id)) then
			discardCardAtRandom(getOpponent(getCardOwner(id)))
		end
	end
}

Cards["Armored Raider Gandaval"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id,"Human")
		local count=Functions.countTappedCreaturesInBattle(getCardOwner(id))
		if(isCardTapped(id)==1) then
			count=count-1
		end
		Abils.PowerAttacker(id,count*2000)
	end
}

Cards["Brad, Super Kickin' Dynamo"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local ability = function(id)
			local ch=createChoice("Choose an opponent's blocker to destroy",0,id,getCardOwner(id),Checks.BlockerInOppBattle)
			if(ch>=0) then
				destroyCreature(ch)
			end
		end
		silentSkill(id,ability)
	end
}

Cards["Burnwisp Lizard"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getCardZone(id)==ZONE_BATTLE) then
			for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do
				if(silentNames[getCardName(card)]) then
					Abils.SpeedAttacker(card)
				end
			end
		end
	end
}

Cards["Colossus Boost"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id)
		local ch=createChoice("Choose one of your creatures",0,id,getCardOwner(id),Checks.InYourBattle)
		if(ch>=0) then
			local ability = function(cid)
				Abils.bonusPower(cid,4000)
			end
			buffUntilEnd(id,ch,ability)
		end
	end
}

Cards["Cragsaur"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Explosive Trooper Zalmez"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			if(getZoneSize(getOpponent(getCardOwner(id)),ZONE_SHIELD)<=2) then
				local valid = function(cid,sid)
					if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=3000) then return 1 end
					return 0
				end
				local ch=createChoice("Choose an opponent's creature to destroy",1,id,getCardOwner(id),valid)
				if(ch>=0) then
					destroyCreature(ch)
				end
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Forced Frenzy"] = {
	price_tier = 1,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local owner=getCardOwner(id)
		for _,card in ipairs(zoneCards(getOpponent(owner),ZONE_BATTLE)) do
			local mod = function(cid,mid)
				Abils.attacksEachTurn(cid)
				if(getMessageType()=="pre startturn" and getMessageInt("player")==owner) then
					destroyModifier(cid,mid)
				end
			end
			createModifier(card,mod)
		end
	end
}

Cards["Hurlosaur"] = {
	price_tier = 3,
	shieldtrigger = 1,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local valid = function(cid,sid)
				if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=1000) then return 1 end
				return 0
			end
			local ch=createChoice("Choose an opponent's creature to destroy",0,id,getCardOwner(id),valid)
			if(ch>=0) then
				destroyCreature(ch)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Mezger, Commando Leader"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.SpeedAttacker(id) end
}

Cards["Minelord Skyterror"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		silentSkill(id,function(id) destroyByPower(id,3000) end)
	end
}

Cards["Mykee's Pliers"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getCardZone(id)==ZONE_BATTLE) then
			for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do
				if(cardHasCivilization(card,CIV_DARKNESS) or cardHasCivilization(card,CIV_NATURE)) then
					Abils.SpeedAttacker(card)
				end
			end
		end
	end
}

Cards["Phantom Dragon's Flame"] = {
	price_tier = 1,
	shieldtrigger = 1,

	OnCast = function(id)
		local valid = function(cid,sid)
			if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=2000) then return 1 end
			return 0
		end
		local ch=createChoice("Choose an opponent's creature to destroy",0,id,getCardOwner(id),valid)
		if(ch>=0) then
			destroyCreature(ch)
		end
	end
}

Cards["Siege Roller Bagash"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local count=Functions.countTappedCreaturesInBattle(getCardOwner(id))
		if(isCardTapped(id)==1) then
			count=count-1
		end
		Abils.PowerAttacker(id,count*1000)
	end
}

Cards["Smash Warrior Stagrandu"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.canAttackUntappedCreatures(id)
		if(getMessageType()=="post creatureattack" and getMessageInt("attacker")==id and getMessageInt("defendertype")==DEFENDER_CREATURE and getCreaturePower(getMessageInt("defender"))>=6000) then
			local ability = function(cid)
				Abils.bonusPower(cid,9000)
			end
			buffUntilEnd(id,id,ability)
		end
	end
}

Cards["Supersonic Jet Pack"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id)
		local ch=createChoice("Choose one of your creatures",0,id,getCardOwner(id),Checks.InYourBattle)
		if(ch>=0) then
			local ability = function(cid)
				Abils.SpeedAttacker(cid)
			end
			buffUntilEnd(id,ch,ability)
		end
	end
}

Cards["Taunting Skyterror"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==1 and getTurn()~=getCardOwner(id)) then
			for _,card in ipairs(zoneCards(getOpponent(getCardOwner(id)),ZONE_BATTLE)) do
				Abils.attacksEachTurn(card)
			end
		end
	end
}

Cards["Vorg's Engine"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		silentSkill(id,function(id) destroyByPower(id,2000) end)
	end
}

Cards["Adventure Boar"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.PowerAttacker(id,2000)
	end
}

Cards["Ancient Horn, the Watcher"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local owner=getCardOwner(id)
			if(getZoneSize(owner,ZONE_SHIELD)>=5) then
				for _,card in ipairs(zoneCards(owner,ZONE_MANA)) do
					untapCard(card)
				end
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Bubble Scarab"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post creatureattack" and getMessageInt("defendertype")==DEFENDER_CREATURE and getCardOwner(getMessageInt("defender"))==getCardOwner(id)) then
			local card=createChoice("Choose a card to discard",1,id,getCardOwner(id),Checks.InYourHand)
			if(card>=0) then
				discardCard(card)
				local ability = function(cid)
					Abils.bonusPower(cid,3000)
				end
				buffUntilEnd(id,getMessageInt("defender"),ability)
			end
		end
	end
}

Cards["Earth Ripper, Talon of Rage"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id,"Beast Folk")
		local summon = function(id)
			for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_MANA)) do
				if(isCardTapped(card)==1) then
					moveCard(card,ZONE_HAND)
				end
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Faerie Life"] = {
	price_tier = 1,
	shieldtrigger = 1,

	OnCast = function(id)
		moveTop(getCardOwner(id),ZONE_MANA)
	end
}

Cards["Hustle Berry"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		silentSkill(id,function(id) moveTop(getCardOwner(id),ZONE_MANA) end)
	end
}

Cards["Jiggly Totem"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		addPowerForTappedMana(id,1000,getCardOwner(id))
	end
}

Cards["Karate Potato"] = {
	price_tier = 2,
	shieldtrigger = 1,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			for i=1,2 do
				local ch=createChoice("Choose a card from your hand to put into mana",1,id,getCardOwner(id),Checks.InYourHand)
				if(ch>=0) then
					moveCard(ch,ZONE_MANA)
				else
					break
				end
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Legacy Shell"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getCardZone(id)==ZONE_BATTLE) then
			for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do
				if(cardHasCivilization(card,CIV_LIGHT) or cardHasCivilization(card,CIV_FIRE)) then
					Abils.PowerAttacker(card,3000)
				end
			end
		end
	end
}

Cards["Sabermask Scarab"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local attack = function(id)
			local ch=createChoice("Choose a card from your mana zone",0,id,getCardOwner(id),Checks.InYourMana)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
		end
		Abils.onAttack(id,attack)
	end
}

Cards["Scowling Tomato"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Shaman Broccoli"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.manaAfterDestroyed(id)
	end
}

Cards["Soulswap"] = {
	price_tier = 3,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local victim=createChoice("Choose a creature to put into its owner's mana zone",1,id,getCardOwner(id),Checks.InBattle)
		if(victim>=0) then
			local player=getCardOwner(victim)
			moveCard(victim,ZONE_MANA)
			local valid = function(cid,sid)
				if(getCardOwner(sid)==player and getCardZone(sid)==ZONE_MANA and getCardType(sid)==TYPE_CREATURE and getCreatureIsEvolution(sid)==0 and getCardCost(sid)<=getZoneSize(player,ZONE_MANA)) then
					return 1
				end
				return 0
			end
			local ch=createChoice("Choose a non-evolution creature from that mana zone",0,id,getCardOwner(id),valid)
			if(ch>=0) then
				moveCard(ch,ZONE_BATTLE)
			end
		end
	end
}

Cards["Sporeblast Erengi"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		silentSkill(id,function(id) searchDeck(id,Checks.CreatureInYourDeck,"Choose a creature from your deck") end)
	end
}

Cards["Terradragon Cusdalf"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.PowerAttacker(id,4000)
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="mod carduntap" and getCardOwner(getMessageInt("card"))==getCardOwner(id) and getCardZone(getMessageInt("card"))==ZONE_MANA and getTurn()==getCardOwner(id)) then
			setMessageInt("msgContinue",0)
		end
	end
}

Cards["Thirst for the Hunt"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id)
		for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do
			local ability = function(cid)
				Abils.PowerAttacker(cid,1000)
			end
			buffUntilEnd(id,card,ability)
		end
	end
}

Cards["Twitch Horn, the Aggressor"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		addPowerForTappedMana(id,2000,getCardOwner(id))
	end
}

Cards["Aqua Skydiver"] = {
	price_tier = 3,
	shieldtrigger = 1,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Blocker(id)
		Abils.returnAfterDestroyed(id)
	end
}

Cards["Estol, Vizier of Aqua"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local summon = function(id)
			moveTop(getCardOwner(id),ZONE_SHIELD)
			local ch=createChoice("Choose an opponent's shield to look at",0,id,getCardOwner(id),Checks.InOppShields)
			if(ch>=0) then
				unflipCard(ch)
				setCardVisibility(ch,getCardOwner(id),1)
				flipCard(ch)
				setCardVisibility(ch,getCardOwner(id),0)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Tajimal, Vizier of Aqua"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Blocker(id)
		Abils.cantAttackPlayers(id)
		local battlingFire = (getAttacker()==id and getDefenderType()==DEFENDER_CREATURE and cardHasCivilization(getDefender(),CIV_FIRE)) or (getDefender()==id and cardHasCivilization(getAttacker(),CIV_FIRE))
		if(getMessageType()=="get creaturepower" and getMessageInt("creature")==id and battlingFire) then
			setMessageInt("power",getMessageInt("power")+4000)
		end
	end
}

Cards["Melnia, the Aqua Shadow"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantBeBlocked(id)
		Abils.Slayer(id)
	end
}

Cards["Pointa, the Aqua Shadow"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local summon = function(id)
			local ch=createChoice("Choose an opponent's shield to look at",0,id,getCardOwner(id),Checks.InOppShields)
			if(ch>=0) then
				unflipCard(ch)
				setCardVisibility(ch,getCardOwner(id),1)
				flipCard(ch)
				setCardVisibility(ch,getCardOwner(id),0)
			end
			discardCardAtRandom(getOpponent(getCardOwner(id)))
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Soderlight, the Cold Blade"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.cantBeBlocked(id)
		local ability = function(id)
			local opponent=getOpponent(getCardOwner(id))
			local ch=createChoice("Choose one of your creatures to destroy",0,id,opponent,Checks.InOppBattle)
			if(ch>=0) then
				destroyCreature(ch)
			end
		end
		silentSkill(id,ability)
	end
}

Cards["Dolmarks, the Shadow Warrior"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local summon = function(id)
			local owner=getCardOwner(id)
			local ch=createChoice("Choose one of your creatures to destroy",0,id,owner,Checks.InYourBattle)
			if(ch>=0) then
				destroyCreature(ch)
			end
			local mana=createChoice("Choose one of your mana cards to destroy",0,id,owner,Checks.InYourMana)
			if(mana>=0) then
				destroyMana(mana)
			end
			local opponent=getOpponent(owner)
			local creature=createChoice("Choose one of your creatures to destroy",0,id,opponent,Checks.InOppBattle)
			if(creature>=0) then
				destroyCreature(creature)
			end
			local oppmana=createChoice("Choose one of your mana cards to destroy",0,id,opponent,Checks.InOppMana)
			if(oppmana>=0) then
				destroyMana(oppmana)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Galek, the Shadow Warrior"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local ch=createChoice("Choose an opponent's blocker to destroy",0,id,getCardOwner(id),Checks.BlockerInOppBattle)
			if(ch>=0) then
				destroyCreature(ch)
			end
			discardCardAtRandom(getOpponent(getCardOwner(id)))
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Ulex, the Dauntless"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="mod cardtap" and getMessageInt("card")==id and getTurn()~=getCardOwner(id)) then
			setMessageInt("msgContinue",0)
		end
	end
}

Cards["Gonta, the Warrior Savage"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Tagtapp, the Retaliator"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local count=0
		for _,card in ipairs(zoneCards(getOpponent(getCardOwner(id)),ZONE_MANA)) do
			if(cardHasCivilization(card,CIV_WATER)) then
				count=count+1
			end
		end
		Abils.bonusPower(id,count*1000)
		if(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id and getCreaturePower(id)>=6000) then
			Abils.Breaker(id,2)
		end
	end
}

Cards["Wind Axe, the Warrior Savage"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local ch=createChoice("Choose an opponent's blocker to destroy",0,id,getCardOwner(id),Checks.BlockerInOppBattle)
			if(ch>=0) then
				destroyCreature(ch)
			end
			moveTop(getCardOwner(id),ZONE_MANA)
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Lukia Lex, Pinnacle Guardian"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.PowerAttacker(id,3000)
		Abils.untapAtEOT(id)
	end
}

Cards["Sanfist, the Savage Vizier"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Blocker(id)
		if(getMessageType()=="mod carddiscard" and getMessageInt("card")==id and getTurn()~=getCardOwner(id)) then
			local use=createChoiceNoCheck("Put Sanfist into the battle zone instead?",2,id,getCardOwner(id),Checks.False)
			if(use==RETURN_BUTTON1) then
				setMessageInt("zoneto",ZONE_BATTLE)
			end
		end
	end
}

Cards["Skysword, the Savage Vizier"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local owner=getCardOwner(id)
			moveTop(owner,ZONE_MANA)
			moveTop(owner,ZONE_SHIELD)
		end
		Abils.onSummon(id,summon)
	end
}
