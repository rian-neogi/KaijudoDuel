package.path = package.path .. ';./?.lua;'
require("Lua/Eternal Wave")

local zoneCards = function(player,zone)
	local cards={}
	for i=0,getZoneSize(player,zone)-1 do
		cards[#cards+1]=getCardAt(player,zone,i)
	end
	return cards
end

local topCard = function(player)
	local size=getZoneSize(player,ZONE_DECK)
	if(size==0) then return -1 end
	return getCardAt(player,ZONE_DECK,size-1)
end

local isEitherRace = function(card,race1,race2)
	return isCreatureOfRace(card,race1)==1 or isCreatureOfRace(card,race2)==1
end

local evolutionEither = function(id,race1,race2)
	if(getMessageType()=="get creatureisevolution" and getMessageInt("creature")==id) then
		setMessageInt("isevolution",1)
	elseif(getMessageType()=="get creaturecanevolve" and getMessageInt("evolution")==id) then
		local bait=getMessageInt("evobait")
		if(isEitherRace(bait,race1,race2)) then setMessageInt("canevolve",1) end
	end
end

local otherRacePowerAura = function(id,race1,race2,power)
	if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="get creaturepower") then
		local creature=getMessageInt("creature")
		if(creature~=id and getCardOwner(creature)==getCardOwner(id) and
			getCardZone(creature)==ZONE_BATTLE and isEitherRace(creature,race1,race2)) then
			setMessageInt("power",getMessageInt("power")+power)
		end
	end
end

local cloneCount = function(name)
	local count=0
	for player=0,1 do
		for _,card in ipairs(zoneCards(player,ZONE_GRAVEYARD)) do
			if(getCardName(card)==name) then count=count+1 end
		end
	end
	return count
end

local chooseClonedTargets = function(id,name,prompt,check,action) --test
	local owner=getCardOwner(id)
	local selected={}
	local choose=function(required)
		local valid=function(cid,sid)
			if(not selected[sid] and check(cid,sid)==1) then return 1 end
			return 0
		end
		local card=createChoice(prompt,required and 0 or 1,id,owner,valid)
		if(card>=0) then selected[card]=true end
		return card
	end
	if(choose(true)<0) then return end
	for i=1,cloneCount(name) do
		if(choose(false)<0) then break end
	end
	for card in pairs(selected) do action(card) end
end

local moveTop = function(player,zone)
	local card=topCard(player)
	if(card>=0) then moveCard(card,zone) end
end

local revealTopForAuraPegasus = function(id) --test
	local owner=getCardOwner(id)
	local card=topCard(owner)
	if(card<0) then return end
	setCardVisibility(card,0,1)
	setCardVisibility(card,1,1)
	createChoiceNoCheck("Reveal the top card of your deck",1,id,owner,Checks.False)
	if(getCardType(card)==TYPE_CREATURE and getCreatureIsEvolution(card)==0) then
		moveCard(card,ZONE_BATTLE)
	else
		moveCard(card,ZONE_HAND)
	end
end

for _,name in ipairs({
	"Kilstine, Nebula Elemental","Steamroller Mutant",
	"Flame Trooper Goliac","Uncanny Turnip"
}) do
	WaveStrikerCards[name]=true
end

Cards["Terradragon Arque Delacerna"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		if(getMessageType()=="mod carddiscard" and getMessageInt("card")==id and
			getCardZone(id)==ZONE_HAND and getTurn()~=getCardOwner(id)) then
			local use=createChoiceNoCheck("Put Terradragon Arque Delacerna into the battle zone instead?",2,id,getCardOwner(id),Checks.False)
			if(use==RETURN_BUTTON1) then setMessageInt("zoneto",ZONE_BATTLE) end
		end
	end
}

Cards["Wise Starnoid, Avatar of Hope"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.VortexEvolution(id,"Light Bringer","Cyber Lord")
		if(getMessageType()=="post creatureattack" and getMessageInt("attacker")==id) then
			moveTop(getCardOwner(id),ZONE_SHIELD)
		elseif(getMessageType()=="post cardmove" and getMessageInt("card")==id and getMessageInt("from")==ZONE_BATTLE) then
			moveTop(getCardOwner(id),ZONE_SHIELD)
		end
	end
}

Cards["Cruel Naga, Avatar of Fate"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.VortexEvolution(id,"Merfolk","Chimera")
		Abils.cantBeBlocked(id)
		if(getMessageType()=="post cardmove" and getMessageInt("card")==id and getMessageInt("from")==ZONE_BATTLE) then
			for player=0,1 do
				for _,card in ipairs(zoneCards(player,ZONE_BATTLE)) do destroyCreature(card) end
			end
		end
	end
}

Cards["Death Phoenix, Avatar of Doom"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.VortexEvolution(id,"Zombie Dragon","Fire Bird")
		if(getMessageType()=="post cardmove" and getMessageInt("card")==id and getMessageInt("from")==ZONE_BATTLE) then
			local opponent=getOpponent(getCardOwner(id))
			discardCardAtRandom(opponent,getZoneSize(opponent,ZONE_HAND))
		end
	end
}

Cards["Aura Pegasus, Avatar of Life"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 3,

	HandleMessage = function(id) --test
		Abils.VortexEvolution(id,"Horned Beast","Angel Command")
		if(getMessageType()=="post creatureattack" and getMessageInt("attacker")==id) then
			revealTopForAuraPegasus(id)
		elseif(getMessageType()=="post cardmove" and getMessageInt("card")==id and getMessageInt("from")==ZONE_BATTLE) then
			revealTopForAuraPegasus(id)
		end
	end
}

Cards["Kilstine, Nebula Elemental"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE and Abils.WaveStrikerActive(id)) then
			local message=getMessageType()
			local creature=message=="get creaturepower" or message=="get creatureisblocker" or message=="get creaturebreaker"
			if(creature) then
				creature=getMessageInt("creature")
				if(creature~=id and getCardOwner(creature)==getCardOwner(id) and getCardZone(creature)==ZONE_BATTLE) then
					Abils.bonusPower(creature,5000)
					Abils.Blocker(creature)
					Abils.Breaker(creature,2)
				end
			end
		end
	end
}

Cards["Extreme Crawler"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.onSummon(id,function(id)
			for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do
				if(card~=id) then moveCard(card,ZONE_HAND) end
			end
		end)
	end
}

Cards["Necrodragon Jagraveen"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 2,

	HandleMessage = function(id) --test
		if(getMessageType()=="post creaturebattle" and getMessageInt("blocked")==1 and
			getMessageInt("defender")==id and getCardZone(id)==ZONE_BATTLE) then
			destroyCreature(id)
		end
	end
}

Cards["Punch Trooper Bronks"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.onSummon(id,function(id)
			local minimum=nil
			for player=0,1 do
				for _,card in ipairs(zoneCards(player,ZONE_BATTLE)) do
					local power=getCreaturePower(card)
					if(minimum==nil or power<minimum) then minimum=power end
				end
			end
			if(minimum==nil) then return end
			local target=createChoice("Choose a creature tied for the lowest power",0,id,getCardOwner(id),function(cid,sid)
				if(Checks.InBattle(cid,sid)==1 and getCreaturePower(sid)==minimum) then return 1 end
				return 0
			end)
			if(target>=0) then destroyCreature(target) end
		end)
	end
}

Cards["Soul Phoenix, Avatar of Unity"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 3,

	HandleMessage = function(id) --test
		Abils.VortexEvolution(id,"Fire Bird","Earth Dragon")
		if(getMessageType()=="mod cardmove" and getMessageInt("card")==id and
			getMessageInt("from")==ZONE_BATTLE and getMessageInt("to")~=ZONE_BATTLE) then
			setMessageInt("separateevolution",1)
		end
	end
}

Cards["Ularus, Punishment Elemental"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.onSummon(id,function(id)
			local count=Functions.countCreaturesInBattle(getCardOwner(id))
			local selected={}
			for i=1,count do
				local shield=createChoice("Choose a shield to turn face up",1,id,getCardOwner(id),function(cid,sid)
					if(getCardZone(sid)==ZONE_SHIELD and not selected[sid]) then return 1 end
					return 0
				end)
				if(shield<0) then break end
				selected[shield]=true
				unflipCard(shield)
				setCardVisibility(shield,0,1)
				setCardVisibility(shield,1,1)
			end
		end)
	end
}

Cards["Cosmic Darts"] = {
	price_tier = 3,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local owner=getCardOwner(id)
		local shield=createChoice("Choose one of the caster's shields",0,id,getOpponent(owner),function(cid,sid)
			if(getCardOwner(sid)==owner and getCardZone(sid)==ZONE_SHIELD) then return 1 end
			return 0
		end)
		if(shield<0) then return end
		unflipCard(shield)
		setCardVisibility(shield,owner,1)
		createChoiceNoCheck("Look at the chosen shield",1,id,owner,Checks.False)
		if(getCardType(shield)==TYPE_SPELL) then
			local cast=createChoiceNoCheck("Cast this spell for no cost?",2,id,owner,Checks.False)
			if(cast==RETURN_BUTTON1) then
				moveCard(shield,ZONE_BATTLE)
				return
			end
		end
		flipCard(shield)
		setCardVisibility(shield,owner,0)
	end
}

Cards["Typhoon Crawler"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="get creaturecanattackcreature" and getMessageInt("defender")==id) then
			local attacker=getMessageInt("attacker")
			if(cardHasCivilization(attacker,CIV_FIRE)==1 or cardHasCivilization(attacker,CIV_NATURE)==1) then
				setMessageInt("canattack",CANATTACK_NO)
			end
		end
	end
}

Cards["Meloppe"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="get shieldchooser") then
			local owner=getCardOwner(id)
			local chooser=getMessageInt("chooser")
			local shieldOwner=getMessageInt("shieldowner")
			if(chooser~=shieldOwner and (chooser==owner or shieldOwner==owner)) then
				setMessageInt("chooser",shieldOwner)
			end
		end
	end
}

Cards["Gigavrand"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="pre endturn") then
			local opponent=getOpponent(getCardOwner(id))
			if(getCardsDrawnThisTurn(opponent)>=2) then
				discardCardAtRandom(opponent,getZoneSize(opponent,ZONE_HAND))
			end
		end
	end
}

Cards["Steamroller Mutant"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.onWaveStrikerSummon(id,function(id)
			for player=0,1 do
				for _,card in ipairs(zoneCards(player,ZONE_BATTLE)) do destroyCreature(card) end
			end
		end)
	end
}

Cards["Whirling Warrior Malian"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post cardmove" and
			getMessageInt("to")==ZONE_BATTLE) then
			local card=getMessageInt("card")
			if(card~=id and getCardType(card)==TYPE_CREATURE) then tapCard(id) end
		end
	end
}

Cards["Mechadragon's Breath"] = {
	price_tier = 3,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local chosen=createChoice("Choose a creature to select a power of 6000 or less",0,id,getCardOwner(id),function(cid,sid)
			if(Checks.InBattle(cid,sid)==1 and getCreaturePower(sid)<=6000) then return 1 end
			return 0
		end)
		if(chosen<0) then return end
		local power=getCreaturePower(chosen)
		for player=0,1 do
			for _,card in ipairs(zoneCards(player,ZONE_BATTLE)) do
				if(getCreaturePower(card)==power) then destroyCreature(card) end
			end
		end
	end
}

Cards["Pincer Scarab"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="get creaturepower") then
			Abils.bonusPower(id,2000*getZoneSize(getOpponent(getCardOwner(id)),ZONE_HAND))
		elseif(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id and getCreaturePower(id)>=6000) then
			Abils.Breaker(id,2)
		end
	end
}

Cards["Radioactive Horn, the Strange"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
	end
}

Cards["Agira, the Warlord Crawler"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		evolutionEither(id,"Gladiator","Earth Eater")
		otherRacePowerAura(id,"Gladiator","Earth Eater",2000)
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post creaturebattle" and getMessageInt("blocked")==1) then
			local blocker=getMessageInt("defender")
			if(getCardOwner(blocker)==getCardOwner(id) and isEitherRace(blocker,"Gladiator","Earth Eater")) then
				local draw=createChoiceNoCheck("Draw a card?",2,id,getCardOwner(id),Checks.False)
				if(draw==RETURN_BUTTON1) then drawCards(getCardOwner(id),1) end
			end
		end
	end
}

Cards["Hydrooze, the Mutant Emperor"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		evolutionEither(id,"Cyber Lord","Hedrian")
		otherRacePowerAura(id,"Cyber Lord","Hedrian",2000)
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="get creaturecanblock") then
			local attacker=getMessageInt("attacker")
			if(getCardOwner(attacker)==getCardOwner(id) and isEitherRace(attacker,"Cyber Lord","Hedrian")) then
				setMessageInt("canblock",0)
			end
		end
	end
}

Cards["Phantomach, the Gigatrooper"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		evolutionEither(id,"Chimera","Armorloid")
		otherRacePowerAura(id,"Chimera","Armorloid",2000)
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="get creaturebreaker") then
			local creature=getMessageInt("creature")
			if(getCardOwner(creature)==getCardOwner(id) and isEitherRace(creature,"Chimera","Armorloid") and
				getMessageInt("breaker")<2) then
				setMessageInt("breaker",2)
			end
		end
	end
}

Cards["Nemonex, Bajula's Robomantis"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		evolutionEither(id,"Xeno Parts","Giant Insect")
		otherRacePowerAura(id,"Xeno Parts","Giant Insect",2000)
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post creatureunblocked") then
			local attacker=getMessageInt("attacker")
			if(getCardOwner(attacker)==getCardOwner(id) and isEitherRace(attacker,"Xeno Parts","Giant Insect")) then
				local opponent=getOpponent(getCardOwner(id))
				local mana=createChoice("Choose one of your mana cards to put into your graveyard",0,id,opponent,function(cid,sid)
					if(getCardOwner(sid)==opponent and getCardZone(sid)==ZONE_MANA) then return 1 end
					return 0
				end)
				if(mana>=0) then destroyMana(mana) end
			end
		end
	end
}

Cards["Comet Eye, The Spectral Spud"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		evolutionEither(id,"Wild Veggies","Rainbow Phantom")
		otherRacePowerAura(id,"Wild Veggies","Rainbow Phantom",2000)
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="pre endturn" and
			getMessageInt("player")==getCardOwner(id)) then
			local selected={}
			while true do
				local creature=createChoice("Choose a creature to untap",1,id,getCardOwner(id),function(cid,sid)
					if(not selected[sid] and getCardOwner(sid)==getCardOwner(cid) and
						getCardZone(sid)==ZONE_BATTLE and isCardTapped(sid)==1 and
						isEitherRace(sid,"Wild Veggies","Rainbow Phantom")) then return 1 end
					return 0
				end)
				if(creature<0) then break end
				selected[creature]=true
				untapCard(creature)
			end
		end
	end
}

Cards["Valkyer, Starstorm Elemental"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttackPlayers(id)
	end
}

Cards["Cloned Deflector"] = {
	price_tier = 2,
	shieldtrigger = 1,

	OnCast = function(id) --test
		chooseClonedTargets(id,"Cloned Deflector","Choose an opponent's creature to tap",Checks.InOppBattle,tapCard)
	end
}

Cards["Cloned Spiral"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id) --test
		chooseClonedTargets(id,"Cloned Spiral","Choose a creature to return to its owner's hand",Checks.InBattle,function(card)
			moveCard(card,ZONE_HAND)
		end)
	end
}

Cards["Enigmatic Cascade"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local owner=getCardOwner(id)
		local selected={}
		while true do
			local card=createChoice("Choose a card to discard",1,id,owner,function(cid,sid)
				if(Checks.InYourHand(cid,sid)==1 and not selected[sid]) then return 1 end
				return 0
			end)
			if(card<0) then break end
			selected[card]=true
			discardCard(card)
		end
		local count=0
		for _ in pairs(selected) do count=count+1 end
		drawCards(owner,count)
	end
}

Cards["Gigabalza"] = {
	price_tier = 2,
	shieldtrigger = 1,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.onSummon(id,function(id)
			discardCardAtRandom(getOpponent(getCardOwner(id)))
		end)
	end
}

Cards["Cloned Nightmare"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local count=1
		for i=1,cloneCount("Cloned Nightmare") do
			local use=createChoiceNoCheck("Discard another random card for a Cloned Nightmare?",2,id,getCardOwner(id),Checks.False)
			if(use~=RETURN_BUTTON1) then break end
			count=count+1
		end
		discardCardAtRandom(getOpponent(getCardOwner(id)),count)
	end
}

Cards["Muramasa's Knife"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.canAttackUntappedCreatures(id)
	end
}

Cards["Cloned Blade"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local valid=function(cid,sid)
			if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=3000) then return 1 end
			return 0
		end
		chooseClonedTargets(id,"Cloned Blade","Choose an opponent's creature with power 3000 or less",valid,destroyCreature)
	end
}

Cards["Wingeye Moth"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post startturndraw" and
			getMessageInt("player")==getCardOwner(id)) then
			local owner=getCardOwner(id)
			local ownMaximum=nil
			local opponentMaximum=nil
			for _,card in ipairs(zoneCards(owner,ZONE_BATTLE)) do
				local power=getCreaturePower(card)
				if(ownMaximum==nil or power>ownMaximum) then ownMaximum=power end
			end
			for _,card in ipairs(zoneCards(getOpponent(owner),ZONE_BATTLE)) do
				local power=getCreaturePower(card)
				if(opponentMaximum==nil or power>opponentMaximum) then opponentMaximum=power end
			end
			if(ownMaximum~=nil and (opponentMaximum==nil or ownMaximum>opponentMaximum)) then
				local draw=createChoiceNoCheck("Draw an extra card?",2,id,owner,Checks.False)
				if(draw==RETURN_BUTTON1) then drawCards(owner,1) end
			end
		end
	end
}

Cards["Cloned Spike-Horn"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="get creaturepower") then
			Abils.bonusPower(id,3000*cloneCount("Cloned Spike-Horn"))
		elseif(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id and getCreaturePower(id)>=6000) then
			Abils.Breaker(id,2)
		end
	end
}

Cards["Electro Explorer Syrion"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Sea Mutant Dormel"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Gigappi Ponto"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Buzz Betocchi"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Spectral Horn Glitalis"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Bingole, the Explorer"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="mod carddiscard" and getMessageInt("card")==id and
			getCardZone(id)==ZONE_HAND and getTurn()~=getCardOwner(id)) then
			local use=createChoiceNoCheck("Put Bingole into the battle zone instead?",2,id,getCardOwner(id),Checks.False)
			if(use==RETURN_BUTTON1) then setMessageInt("zoneto",ZONE_BATTLE) end
		end
	end
}

Cards["Mizoy, the Oracle"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.onSummon(id,function(id)
			local creature=createChoice("Choose a darkness or fire creature to tap",1,id,getCardOwner(id),function(cid,sid)
				if(Checks.InBattle(cid,sid)==1 and
					(cardHasCivilization(sid,CIV_DARKNESS)==1 or cardHasCivilization(sid,CIV_FIRE)==1)) then return 1 end
				return 0
			end)
			if(creature>=0) then tapCard(creature) end
		end)
	end
}

Cards["Belmol, the Explorer"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.cantAttackPlayers(id)
		if(getMessageType()=="get creatureforcedblocker" and getCardZone(id)==ZONE_BATTLE and
			isCardTapped(id)==0 and getCardOwner(id)~=getCardOwner(getMessageInt("attacker")) and
			getMessageInt("forcedblocker")<0) then
			setMessageInt("forcedblocker",id)
		end
	end
}

Cards["Pharzi, the Oracle"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.onDestroy(id,function(id)
			local spell=createChoice("Choose a spell in your graveyard",1,id,getCardOwner(id),Checks.SpellInYourGraveyard)
			if(spell>=0) then moveCard(spell,ZONE_HAND) end
		end)
	end
}

Cards["Tropic Crawler"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.cantAttack(id)
		if(getMessageType()=="post creaturebattle" and getMessageInt("blocked")==1 and
			getMessageInt("defender")==id) then
			local opponent=getOpponent(getCardOwner(id))
			local creature=createChoice("Choose one of your creatures to return to your hand",0,id,opponent,function(cid,sid)
				if(getCardOwner(sid)==opponent and getCardZone(sid)==ZONE_BATTLE and getCardType(sid)==TYPE_CREATURE) then return 1 end
				return 0
			end)
			if(creature>=0) then moveCard(creature,ZONE_HAND) end
		end
	end
}

Cards["Funky Wizard"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.onSummon(id,function(id)
			for player=0,1 do
				local draw=createChoiceNoCheck("Draw a card?",2,id,player,Checks.False)
				if(draw==RETURN_BUTTON1) then drawCards(player,1) end
			end
		end)
	end
}

Cards["Wily Carpenter"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.onSummon(id,function(id)
			local owner=getCardOwner(id)
			for i=1,2 do
				local draw=createChoiceNoCheck("Draw a card?",2,id,owner,Checks.False)
				if(draw~=RETURN_BUTTON1) then break end
				drawCards(owner,1)
			end
			for i=1,2 do
				local card=createChoice("Choose a card to discard",0,id,owner,Checks.InYourHand)
				if(card<0) then break end
				discardCard(card)
			end
		end)
	end
}

Cards["Frantic Chieftain"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.onSummon(id,function(id)
			local creature=createChoice("Choose one of your creatures that costs 4 or less",0,id,getCardOwner(id),function(cid,sid)
				if(Checks.InYourBattle(cid,sid)==1 and getCardCost(sid)<=4) then return 1 end
				return 0
			end)
			if(creature>=0) then moveCard(creature,ZONE_HAND) end
		end)
	end
}

Cards["Necrodragon Zalva"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.onSummon(id,function(id)
			drawCards(getOpponent(getCardOwner(id)),1)
		end)
	end
}

Cards["Gigarayze"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.onSummon(id,function(id)
			local creature=createChoice("Choose a water or fire creature in your graveyard",1,id,getCardOwner(id),function(cid,sid)
				if(Checks.CreatureInYourGraveyard(cid,sid)==1 and
					(cardHasCivilization(sid,CIV_WATER)==1 or cardHasCivilization(sid,CIV_FIRE)==1)) then return 1 end
				return 0
			end)
			if(creature>=0) then moveCard(creature,ZONE_HAND) end
		end)
	end
}

Cards["Windmill Mutant"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.discardOppCardOnAttack(id,1)
	end
}

Cards["Gigaslug"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttack(id)
		Abils.Slayer(id)
	end
}

Cards["Flame Trooper Goliac"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.onWaveStrikerSummon(id,function(id)
			local creature=createChoice("Choose an opponent's creature with power 5000 or less",0,id,getCardOwner(id),function(cid,sid)
				if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=5000) then return 1 end
				return 0
			end)
			if(creature>=0) then destroyCreature(creature) end
		end)
	end
}

Cards["Hypersprint Warior Uzesol"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.SpeedAttacker(id)
		Abils.PowerAttacker(id,4000)
	end
}

Cards["Peppi Pepper"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.PowerAttacker(id,3000)
	end
}

Cards["Gandaval's Stapler"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post cardmove" and
			getMessageInt("to")==ZONE_BATTLE) then
			local card=getMessageInt("card")
			if(card~=id and getCardType(card)==TYPE_CREATURE) then tapCard(id) end
		end
	end
}

Cards["Copper Locust"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post creatureevolve" and
			getMessageInt("evolution")~=id) then
			destroyCreature(id)
		end
	end
}

Cards["Turtle Horn, the Imposing"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post shieldtriggerused") then
			local trigger=getMessageInt("trigger")
			if(getCardOwner(trigger)~=getCardOwner(id)) then
				local creature=createChoice("Choose a creature from your deck",1,id,getCardOwner(id),Checks.CreatureInYourDeck)
				if(creature>=0) then moveCard(creature,ZONE_HAND) end
				shuffleDeck(getCardOwner(id))
			end
		end
	end
}

Cards["Fever Nuts"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="get cardcost") then
			local card=getMessageInt("card")
			if(getCardType(card)==TYPE_CREATURE and getMessageInt("cost")>1) then
				setMessageInt("cost",getMessageInt("cost")-1)
			end
		end
	end
}

Cards["Uncanny Turnip"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.onWaveStrikerSummon(id,function(id)
			local owner=getCardOwner(id)
			moveTop(owner,ZONE_MANA)
			local creature=createChoice("Choose a creature in your mana zone to return to your hand",0,id,owner,Checks.CreatureInYourMana)
			if(creature>=0) then moveCard(creature,ZONE_HAND) end
		end)
	end
}
