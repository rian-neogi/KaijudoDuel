package.path = package.path .. ';./?.lua;'
require("Lua/Eternal Arms")

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

local waveStrikerCards = WaveStrikerCards
for _,name in ipairs({
	"Klujadras","Asra, Vizier of Safety","Lamiel, Destiny Enforcer",
	"Merlee, the Oracle","Aqua Trickster","Revival Soldier",
	"Hazaria, Duke of Thorns","Jagila, the Hidden Pillager","Saliva Worm",
	"Bonfire Lizard","Eviscerating Warrior Lumez","Sapian Tark, Flame Dervish",
	"Macho Melon","Ninja Pumpkin","Skyscraper Shell"
}) do
	waveStrikerCards[name]=true
end

local waveStrikerActive = Abils.WaveStrikerActive
local onWaveStrikerSummon = Abils.onWaveStrikerSummon

local silentSkill = function(id,ability)
	if(getMessageType()=="pre startturn" and getMessageInt("player")==getCardOwner(id) and getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==1) then
		local use=createChoiceNoCheck("Use this creature's silent skill?",2,id,getCardOwner(id),Checks.False)
		if(use==RETURN_BUTTON1) then
			local skipUntap=function(cid,mid)
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

local buffUntilEnd = function(target,ability)
	local mod=function(cid,mid)
		ability(cid)
		if(getMessageType()=="pre endturn") then
			destroyModifier(cid,mid)
		end
	end
	createModifier(target,mod)
end

local chooseCivilization = function(id) --test
	local names={"Light","Nature","Water","Fire"}
	local civilizations={CIV_LIGHT,CIV_NATURE,CIV_WATER,CIV_FIRE}
	for i=1,#names do
		local choice=createChoiceNoCheck("Choose "..names[i].."? (No checks the next civilization)",2,id,getCardOwner(id),Checks.False)
		if(choice==RETURN_BUTTON1) then
			return civilizations[i]
		end
	end
	return CIV_DARKNESS
end

local chooseCards = function(prompt,count,id,player,check,required)
	local chosen = {}
	for i=1,count do
		local valid=function(cid,sid)
			if(chosen[sid]) then return 0 end
			return check(cid,sid)
		end
		local card=createChoice(prompt,required and 0 or 1,id,player,valid)
		if(card<0) then break end
		chosen[card]=true
	end
	return chosen
end

local setHandVisibility = function(player,viewer,visible)
	for _,card in ipairs(zoneCards(player,ZONE_HAND)) do
		setCardVisibility(card,viewer,visible)
	end
end

Cards["Warlord Ailzonius"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id,"Gladiator")
		if(getMessageType()=="get creaturecanbechosen" and getMessageInt("creature")==id and getMessageInt("chooser")~=getCardOwner(id)) then
			setMessageInt("canchoose",0)
		end
	end
}

Cards["Klujadras"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		onWaveStrikerSummon(id,function(id)
			for player=0,1 do
				local count=0
				for _,card in ipairs(zoneCards(player,ZONE_BATTLE)) do
					if(waveStrikerCards[getCardName(card)]) then count=count+1 end
				end
				drawCards(player,count)
			end
		end)
	end
}

Cards["Miraculous Plague"] = {
	price_tier = 5,
	shieldtrigger = 0,

	HandleMessage = function(id)
		Abils.AiCanCastIfValidTarget(id,getOpponent(getCardOwner(id)),ZONE_BATTLE,Checks.InOppBattle)
	end,

	OnCast = function(id) --test
		local owner=getCardOwner(id)
		local resolvePair=function(prompt,check,destroy)
			local selected=chooseCards(prompt,2,id,owner,check,true)
			local count=0
			for _ in pairs(selected) do count=count+1 end
			if(count==0) then return end
			local valid=function(cid,sid)
				if(selected[sid]) then return 1 end
				return 0
			end
			local returned=createChoice("Choose one card to return to your hand",0,id,getOpponent(owner),valid)
			if(returned>=0) then
				moveCard(returned,ZONE_HAND)
				selected[returned]=nil
			end
			for card in pairs(selected) do destroy(card) end
		end
		resolvePair("Choose an opponent's creature",Checks.InOppBattle,destroyCreature)
		resolvePair("Choose a card in your opponent's mana zone",Checks.InOppMana,destroyMana)
	end
}

Cards["Miraculous Meltdown"] = {
	price_tier = 5,
	shieldtrigger = 0,

	HandleMessage = function(id)
		if(getMessageType()=="get cardcancast" and getMessageInt("card")==id) then
			local owner=getCardOwner(id)
			if(getZoneSize(getOpponent(owner),ZONE_SHIELD)<=getZoneSize(owner,ZONE_SHIELD)) then
				setMessageInt("cancast",0)
			end
		end
	end,

	OnCast = function(id) --test
		local owner=getCardOwner(id)
		local opponent=getOpponent(owner)
		local keepCount=getZoneSize(owner,ZONE_SHIELD)
		local shields=zoneCards(opponent,ZONE_SHIELD)
		local kept={}
		for i=1,keepCount do
			local valid=function(cid,sid)
				if(Checks.InOppShields(cid,sid)==1 and not kept[sid]) then return 1 end
				return 0
			end
			local shield=createChoice("Choose a shield to keep",0,id,opponent,valid)
			if(shield<0) then break end
			kept[shield]=true
		end
		for _,shield in ipairs(shields) do
			if(not kept[shield]) then
				moveCard(shield,ZONE_HAND)
				if(getCardIsShieldTrigger(shield)==1) then
					local use=createChoiceNoCheck("Use this shield trigger?",2,id,opponent,Checks.False)
					if(use==RETURN_BUTTON1) then moveCard(shield,ZONE_BATTLE) end
				end
			end
		end
	end
}

Cards["Miraculous Rebirth"] = {
	price_tier = 5,
	shieldtrigger = 0,

	HandleMessage = function(id)
		Abils.AiCanCastIfValidTarget(id,getOpponent(getCardOwner(id)),ZONE_BATTLE,function(cid,sid)
			if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=5000) then return 1 end
			return 0
		end)
	end,

	OnCast = function(id) --test
		local target=createChoice("Choose an opponent's creature with power 5000 or less",0,id,getCardOwner(id),function(cid,sid)
			if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=5000) then return 1 end
			return 0
		end)
		if(target>=0) then
			local cost=getCardCost(target)
			destroyCreature(target)
			local creature=createChoice("Choose a creature with the same cost from your deck",1,id,getCardOwner(id),function(cid,sid)
				if(Checks.CreatureInYourDeck(cid,sid)==1 and getCardCost(sid)==cost) then return 1 end
				return 0
			end)
			if(creature>=0) then moveCard(creature,ZONE_BATTLE) end
			shuffleDeck(getCardOwner(id))
		end
	end
}

Cards["Evil Incarnate"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.Evolution(id,"Devil Mask")
		if(getMessageType()=="pre startturn" and getCardZone(id)==ZONE_BATTLE) then
			local player=getMessageInt("player")
			local target=createChoice("Choose one of your creatures to destroy",0,id,player,function(cid,sid)
				if(getCardOwner(sid)==player and getCardZone(sid)==ZONE_BATTLE and getCardType(sid)==TYPE_CREATURE) then return 1 end
				return 0
			end)
			if(target>=0) then destroyCreature(target) end
		end
	end
}

Cards["Heavyweight Dragon"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.TapAbility(id,function(id)
			local selected={}
			local total=0
			for i=1,2 do
				local target=createChoice("Choose a tapped creature with lower combined power",1,id,getCardOwner(id),function(cid,sid)
					if(Checks.InOppBattle(cid,sid)==1 and isCardTapped(sid)==1 and not selected[sid] and total+getCreaturePower(sid)<getCreaturePower(id)) then return 1 end
					return 0
				end)
				if(target<0) then break end
				selected[target]=true
				total=total+getCreaturePower(target)
			end
			for card in pairs(selected) do destroyCreature(card) end
		end)
	end
}

Cards["Diamondia, the Blizzard Rider"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Evolution(id,"Snow Faerie")
		Abils.onSummon(id,function(id)
			local owner=getCardOwner(id)
			for _,zone in ipairs({ZONE_GRAVEYARD,ZONE_MANA}) do
				for _,card in ipairs(zoneCards(owner,zone)) do
					if(getCardType(card)==TYPE_CREATURE and isCreatureOfRace(card,"Snow Faerie")==1) then moveCard(card,ZONE_HAND) end
				end
			end
		end)
	end
}

Cards["Miraculous Snare"] = {
	price_tier = 5,
	shieldtrigger = 0,

	HandleMessage = function(id)
		Abils.AiCanCastIfValidTarget(id,getOpponent(getCardOwner(id)),ZONE_BATTLE,function(cid,sid)
			if(Checks.InBattle(cid,sid)==1 and getCreatureIsEvolution(sid)==0) then return 1 end
			return 0
		end)
	end,

	OnCast = function(id)
		local target=createChoice("Choose a non-evolution creature",0,id,getCardOwner(id),function(cid,sid)
			if(Checks.InBattle(cid,sid)==1 and getCreatureIsEvolution(sid)==0) then return 1 end
			return 0
		end)
		if(target>=0) then moveCard(target,ZONE_SHIELD) end
	end
}

Cards["Miraculous Truce"] = {
	price_tier = 5,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local owner=getCardOwner(id)
		local civilization=chooseCivilization(id)
		local mod=function(cid,mid)
			if(getMessageType()=="get creaturecanattackplayers") then
				local attacker=getMessageInt("attacker")
				if(getCardOwner(attacker)~=owner and cardHasCivilization(attacker,civilization)==1) then
					setMessageInt("canattack",CANATTACK_NO)
				end
			elseif(getMessageType()=="pre startturn" and getMessageInt("player")==owner) then
				destroyModifier(cid,mid)
			end
		end
		createModifier(id,mod)
	end
}

Cards["Asra, Vizier of Safety"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(waveStrikerActive(id)) then
			Abils.bonusPower(id,4000)
			Abils.Blocker(id)
		end
	end
}

Cards["Baraid, the Explorer"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		silentSkill(id,function(id)
			local owner=getCardOwner(id)
			buffUntilEnd(id,function(cid)
				if(getMessageType()=="get creaturecanblock") then
					local attacker=getMessageInt("attacker")
					if(getCardOwner(attacker)==owner and cardHasCivilization(attacker,CIV_LIGHT)==1) then setMessageInt("canblock",0) end
				end
			end)
		end)
	end
}

Cards["Belix, the Explorer"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttackPlayers(id)
		Abils.onSummon(id,function(id)
			local card=createChoice("Choose a spell in your mana zone",1,id,getCardOwner(id),Checks.SpellInYourMana)
			if(card>=0) then moveCard(card,ZONE_HAND) end
		end)
	end
}

Cards["Engbelt, the Spydroid"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttackPlayers(id)
	end
}

Cards["Lamiel, Destiny Enforcer"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(waveStrikerActive(id) and getMessageType()=="post creaturedestroy" and getCardZone(id)==ZONE_BATTLE) then
			local creature=getMessageInt("creature")
			local owner=getCardOwner(id)
			if(getCardOwner(creature)==owner and getTurn()~=owner) then
				local draw=createChoiceNoCheck("Draw a card?",2,id,owner,Checks.False)
				if(draw==RETURN_BUTTON1) then drawCards(owner,1) end
			end
		end
	end
}

Cards["Merlee, the Oracle"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(waveStrikerActive(id) and getCardZone(id)==ZONE_BATTLE and getMessageType()=="get creaturepower") then
			local creature=getMessageInt("creature")
			if(getCardOwner(creature)==getCardOwner(id) and getCardZone(creature)==ZONE_BATTLE) then
				setMessageInt("power",getMessageInt("power")+1000)
			end
		end
	end
}

Cards["Nial, Vizier of Dexterity"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.untapAtEOT(id)
	end
}

Cards["Solar Trap"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id)
		local target=createChoice("Choose an opponent's creature to tap",0,id,getCardOwner(id),Checks.InOppBattle)
		if(target>=0) then tapCard(target) end
	end
}

Cards["Yuliana, Channeler of Suns"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttackPlayers(id)
		if(getMessageType()=="get creaturecanbechosen" and getMessageInt("creature")==id and getMessageInt("chooser")~=getCardOwner(id)) then
			setMessageInt("canchoose",0)
		end
	end
}

Cards["Aqua Trickster"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		onWaveStrikerSummon(id,function(id)
			local target=createChoice("Choose an opponent's creature to tap",0,id,getCardOwner(id),Checks.InOppBattle)
			if(target>=0) then tapCard(target) end
		end)
	end
}

Cards["Emergency Typhoon"] = {
	price_tier = 1,
	shieldtrigger = 1,

	OnCast = function(id)
		local owner=getCardOwner(id)
		for i=1,2 do
			local draw=createChoiceNoCheck("Draw a card?",2,id,owner,Checks.False)
			if(draw~=RETURN_BUTTON1) then break end
			drawCards(owner,1)
		end
		local card=createChoice("Choose a card to discard",0,id,owner,Checks.InYourHand)
		if(card>=0) then discardCard(card) end
	end
}

Cards["Fantasy Fish"] = {
	price_tier = 3,
	shieldtrigger = 1,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Lucky Ball"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.onSummon(id,function(id)
			local owner=getCardOwner(id)
			if(getZoneSize(getOpponent(owner),ZONE_SHIELD)<=3) then
				for i=1,2 do
					local draw=createChoiceNoCheck("Draw a card?",2,id,owner,Checks.False)
					if(draw~=RETURN_BUTTON1) then break end
					drawCards(owner,1)
				end
			end
		end)
	end
}

Cards["Melodic Hunter"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Revival Soldier"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(waveStrikerActive(id)) then
			Abils.bonusPower(id,4000)
			Abils.returnAfterDestroyed(id)
		end
	end
}

Cards["Squawking Lunatron"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		silentSkill(id,function(id)
			local selected=chooseCards("Choose a card in your mana zone to return",3,id,getCardOwner(id),Checks.InYourMana)
			for card in pairs(selected) do moveCard(card,ZONE_HAND) end
		end)
	end
}

Cards["Time Scout"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.onSummon(id,function(id)
			local owner=getCardOwner(id)
			local card=topCard(getOpponent(owner))
			if(card>=0) then
				setCardVisibility(card,owner,1)
				createChoiceNoCheck("Look at the top card of your opponent's deck",1,id,owner,Checks.False)
				setCardVisibility(card,owner,0)
			end
		end)
	end
}

Cards["Warped Lunatron"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="pre startturn") then
			local player=getMessageInt("player")
			for _,creature in ipairs(zoneCards(player,ZONE_BATTLE)) do
				local skip=function(cid,mid)
					if(getMessageType()=="mod carduntap" and getMessageInt("card")==cid) then
						setMessageInt("msgContinue",0)
						destroyModifier(cid,mid)
					end
				end
				createModifier(creature,skip)
			end
		elseif(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post startturn") then
			local player=getMessageInt("player")
			while true do
				local first=createChoice("Choose the first mana card to tap",1,id,player,function(cid,sid)
					if(getCardOwner(sid)==player and getCardZone(sid)==ZONE_MANA and isCardTapped(sid)==0) then return 1 end
					return 0
				end)
				if(first<0) then break end
				local second=createChoice("Choose the second mana card to tap",0,id,player,function(cid,sid)
					if(sid~=first and getCardOwner(sid)==player and getCardZone(sid)==ZONE_MANA and isCardTapped(sid)==0) then return 1 end
					return 0
				end)
				if(second<0) then break end
				tapCard(first)
				tapCard(second)
				local creature=createChoice("Choose one of your creatures to untap",0,id,player,function(cid,sid)
					if(getCardOwner(sid)==player and getCardZone(sid)==ZONE_BATTLE and isCardTapped(sid)==1) then return 1 end
					return 0
				end)
				if(creature>=0) then untapCard(creature) end
			end
		end
	end
}

Cards["Baira, the Hidden Lunatic"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttack(id)
		Abils.destroyAfterBattle(id)
	end
}

Cards["Beratcha, the Hidden Glutton"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Slayer(id)
	end
}

Cards["Gazer Eyes, Shadow of Secrets"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		silentSkill(id,function(id)
			local owner=getCardOwner(id)
			local opponent=getOpponent(owner)
			setHandVisibility(opponent,owner,1)
			local card=createChoice("Choose a card from your opponent's hand",0,id,owner,Checks.InOppHand)
			if(card>=0) then discardCard(card) end
			setHandVisibility(opponent,owner,0)
		end)
	end
}

Cards["Hazaria, Duke of Thorns"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		onWaveStrikerSummon(id,function(id)
			local opponent=getOpponent(getCardOwner(id))
			local target=createChoice("Choose one of your creatures to destroy",0,id,opponent,function(cid,sid)
				if(getCardOwner(sid)==opponent and getCardZone(sid)==ZONE_BATTLE and getCardType(sid)==TYPE_CREATURE) then return 1 end
				return 0
			end)
			if(target>=0) then destroyCreature(target) end
		end)
	end
}

Cards["Jagila, the Hidden Pillager"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		onWaveStrikerSummon(id,function(id)
			discardCardAtRandom(getOpponent(getCardOwner(id)),3)
		end)
	end
}

Cards["Morbid Medicine"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id)
		local selected=chooseCards("Choose a creature in your graveyard",2,id,getCardOwner(id),Checks.CreatureInYourGraveyard)
		for card in pairs(selected) do moveCard(card,ZONE_HAND) end
	end
}

Cards["Roulette of Ruin"] = {
	price_tier = 3,
	shieldtrigger = 1,

	HandleMessage = function(id)
		Abils.AiCanCastIfOpponentHasHand(id)
	end,

	OnCast = function(id) --test
		local owner=getCardOwner(id)
		local opponent=getOpponent(owner)
		setHandVisibility(opponent,owner,1)
		setHandVisibility(owner,opponent,1)
		local chosen=createChoice("Choose a card to select its mana cost",1,id,owner,function(cid,sid)
			if(getCardZone(sid)==ZONE_HAND) then return 1 end
			return 0
		end)
		if(chosen>=0) then
			local cost=getCardCost(chosen)
			for player=0,1 do
				for _,card in ipairs(zoneCards(player,ZONE_HAND)) do
					if(getCardCost(card)==cost) then discardCard(card) end
				end
			end
		end
		setHandVisibility(opponent,owner,0)
		setHandVisibility(owner,opponent,0)
	end
}

Cards["Saliva Worm"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(waveStrikerActive(id)) then
			Abils.bonusPower(id,4000)
			Abils.Stealth(id,CIV_DARKNESS)
		end
	end
}

Cards["Spinning Terror, the Wretched"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="get creaturepower" and getMessageInt("creature")==id and getCardZone(id)==ZONE_BATTLE) then
			setMessageInt("power",getMessageInt("power")+2000*Functions.countTappedCreaturesInBattle(getOpponent(getCardOwner(id))))
		end
	end
}

Cards["Bonfire Lizard"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		onWaveStrikerSummon(id,function(id)
			local selected=chooseCards("Choose an opponent's blocker to destroy",2,id,getCardOwner(id),Checks.BlockerInOppBattle)
			for card in pairs(selected) do destroyCreature(card) end
		end)
	end
}

Cards["Brad's Cutter"] = {
	price_tier = 1,
	shieldtrigger = 1,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Eviscerating Warrior Lumez"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		onWaveStrikerSummon(id,function(id)
			for player=0,1 do
				for _,card in ipairs(zoneCards(player,ZONE_BATTLE)) do
					if(getCreaturePower(card)<=2000) then destroyCreature(card) end
				end
			end
		end)
	end
}

Cards["Gankloak, Rogue Commando"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		silentSkill(id,function(id)
			local owner=getCardOwner(id)
			buffUntilEnd(id,function(cid)
				if(getMessageType()=="get creaturebreaker") then
					local creature=getMessageInt("creature")
					if(getCardOwner(creature)==owner and cardHasCivilization(creature,CIV_FIRE)==1 and getMessageInt("breaker")<2) then setMessageInt("breaker",2) end
				end
			end)
		end)
	end
}

Cards["Hysteria Lizard"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.attacksEachTurn(id)
		Abils.PowerAttacker(id,3000)
	end
}

Cards["Jabaha's Automaton"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id,"XenoParts")
		Abils.PowerAttacker(id,4000)
	end
}

Cards["Lockdown Lizard"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Sapian Tark, Flame Dervish"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(waveStrikerActive(id)) then
			Abils.bonusPower(id,4000)
			Abils.canAttackUntappedCreatures(id)
		end
	end
}

Cards["Ten-Ton Crunch"] = {
	price_tier = 1,
	shieldtrigger = 1,

	HandleMessage = function(id)
		Abils.AiCanCastIfValidTarget(id,getOpponent(getCardOwner(id)),ZONE_BATTLE,function(cid,sid)
			if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=3000) then return 1 end
			return 0
		end)
	end,

	OnCast = function(id)
		local target=createChoice("Choose an opponent's creature with power 3000 or less",0,id,getCardOwner(id),function(cid,sid)
			if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=3000) then return 1 end
			return 0
		end)
		if(target>=0) then destroyCreature(target) end
	end
}

Cards["Hazard Hopper"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="pre endturn" and getMessageInt("player")==getCardOwner(id) and getCardZone(id)==ZONE_BATTLE and hasCreatureBrokenShieldThisTurn(id)==1) then
			moveCard(id,ZONE_HAND)
		end
	end
}

Cards["Hearty Cap'n Polligon"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="pre endturn" and getMessageInt("player")==getCardOwner(id) and getCardZone(id)==ZONE_BATTLE and hasCreatureBrokenShieldThisTurn(id)==1) then
			moveCard(id,ZONE_HAND)
		end
	end
}

Cards["Macho Melon"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(waveStrikerActive(id)) then Abils.PowerAttacker(id,3000) end
	end
}

Cards["Ninja Pumpkin"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(waveStrikerActive(id)) then
			Abils.bonusPower(id,4000)
			Abils.cantBeBlockedPower(id,5000)
		end
	end
}

Cards["Quillspike Rumbler"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="post creatureattack" and getMessageInt("attacker")==id and getDefenderType()==DEFENDER_CREATURE) then
			buffUntilEnd(id,function(cid) Abils.bonusPower(cid,3000) end)
		end
	end
}

Cards["Rainbow Gate"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id)
		local owner=getCardOwner(id)
		local valid=function(cid,sid)
			if(Checks.CreatureInYourDeck(cid,sid)==0) then return 0 end
			local count=0
			for civ=0,4 do
				if(cardHasCivilization(sid,civ)==1) then count=count+1 end
			end
			if(count>=2) then return 1 end
			return 0
		end
		local preferred=Functions.HighestCostChoice(id,owner,ZONE_DECK,valid)
		local creature=createChoice("Choose a multicolored creature from your deck",1,id,owner,valid,preferred)
		if(creature>=0) then moveCard(creature,ZONE_HAND) end
		shuffleDeck(getCardOwner(id))
	end
}

Cards["Rollicking Totem"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		silentSkill(id,function(id)
			local dragon=createChoice("Choose a Dragon in your mana zone",0,id,getCardOwner(id),function(cid,sid)
				if(Checks.CreatureInYourMana(cid,sid)==1 and isCreatureOfRace(sid,"Dragon")==1) then return 1 end
				return 0
			end)
			if(dragon>=0) then moveCard(dragon,ZONE_BATTLE) end
		end)
	end
}

Cards["Royal Durian"] = {
	price_tier = 3,
	shieldtrigger = 1,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		silentSkill(id,function(id)
			local dragon=createChoice("Choose a Dragon in your mana zone",0,id,getCardOwner(id),function(cid,sid)
				if(Checks.CreatureInYourMana(cid,sid)==1 and isCreatureOfRace(sid,"Dragon")==1) then return 1 end
				return 0
			end)
			if(dragon>=0) then moveCard(dragon,ZONE_BATTLE) end
		end)
	end
}

Cards["Skyscraper Shell"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		onWaveStrikerSummon(id,function(id)
			local opponent=getOpponent(getCardOwner(id))
			local target=createChoice("Choose one of your creatures to put into your mana zone",0,id,opponent,function(cid,sid)
				if(getCardOwner(sid)==opponent and getCardZone(sid)==ZONE_BATTLE and getCardType(sid)==TYPE_CREATURE) then return 1 end
				return 0
			end)
			if(target>=0) then moveCard(target,ZONE_MANA) end
		end)
	end
}

Cards["Rise and Shine"] = {
	price_tier = 3,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local owner=getCardOwner(id)
		local revealed={}
		local deck=zoneCards(owner,ZONE_DECK)
		for i=#deck,math.max(1,#deck-3),-1 do
			local card=deck[i]
			revealed[card]=true
			setCardVisibility(card,owner,1)
		end
		local blocker=createChoice("Choose a blocker to put into your hand",1,id,owner,function(cid,sid)
			if(revealed[sid] and getCardType(sid)==TYPE_CREATURE and getCreatureIsBlocker(sid)==1) then return 1 end
			return 0
		end)
		if(blocker>=0) then
			moveCard(blocker,ZONE_HAND)
			revealed[blocker]=nil
		end
		for card in pairs(revealed) do
			setCardVisibility(card,owner,0)
			moveCard(card,ZONE_DECK,1)
		end
	end
}

Cards["Live and Breathe"] = {
	price_tier = 3,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local owner=getCardOwner(id)
		local mod=function(cid,mid)
			if(getMessageType()=="post cardmove" and getMessageInt("to")==ZONE_BATTLE) then
				local summoned=getMessageInt("card")
				if(getCardOwner(summoned)==owner and getCardType(summoned)==TYPE_CREATURE) then
					local name=getCardName(summoned)
					local copy=createChoice("Choose another "..name.." from your deck",1,cid,owner,function(source,sid)
						if(Checks.CreatureInYourDeck(source,sid)==1 and getCardName(sid)==name) then return 1 end
						return 0
					end)
					if(copy>=0) then moveCard(copy,ZONE_BATTLE) end
					shuffleDeck(owner)
				end
			elseif(getMessageType()=="pre endturn" and getMessageInt("player")==owner) then
				destroyModifier(cid,mid)
			end
		end
		createModifier(id,mod)
	end
}

Cards["Hide and Seek"] = {
	price_tier = 3,
	shieldtrigger = 0,

	HandleMessage = function(id)
		Abils.AiCanCastIfValidTarget(id,getOpponent(getCardOwner(id)),ZONE_BATTLE,function(cid,sid)
			if(Checks.InOppBattle(cid,sid)==1 and getCreatureIsEvolution(sid)==0) then return 1 end
			return 0
		end)
	end,

	OnCast = function(id)
		local target=createChoice("Choose an opponent's non-evolution creature",0,id,getCardOwner(id),function(cid,sid)
			if(Checks.InOppBattle(cid,sid)==1 and getCreatureIsEvolution(sid)==0) then return 1 end
			return 0
		end)
		if(target>=0) then moveCard(target,ZONE_HAND) end
		discardCardAtRandom(getOpponent(getCardOwner(id)))
	end
}

Cards["Slash and Burn"] = {
	price_tier = 3,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local owner=getCardOwner(id)
		local mod=function(cid,mid)
			if(getMessageType()=="post creaturedestroy") then
				local creature=getMessageInt("creature")
				if(getCardOwner(creature)~=owner) then
					local opponent=getOpponent(owner)
					local mana=createChoice("Choose one of your mana cards to put into your graveyard",0,cid,opponent,function(source,sid)
						if(getCardOwner(sid)==opponent and getCardZone(sid)==ZONE_MANA) then return 1 end
						return 0
					end)
					if(mana>=0) then destroyMana(mana) end
					local shield=createChoice("Choose one of your shields to put into your graveyard",0,cid,opponent,function(source,sid)
						if(getCardOwner(sid)==opponent and getCardZone(sid)==ZONE_SHIELD) then return 1 end
						return 0
					end)
					if(shield>=0) then moveCard(shield,ZONE_GRAVEYARD) end
				end
			elseif(getMessageType()=="pre endturn" and getMessageInt("player")==owner) then
				destroyModifier(cid,mid)
			end
		end
		createModifier(id,mod)
	end
}

Cards["Reap and Sow"] = {
	price_tier = 3,
	shieldtrigger = 0,

	OnCast = function(id)
		local mana=createChoice("Choose a card in your opponent's mana zone",0,id,getCardOwner(id),Checks.InOppMana)
		if(mana>=0) then destroyMana(mana) end
		local top=topCard(getCardOwner(id))
		if(top>=0) then moveCard(top,ZONE_MANA) end
	end
}
