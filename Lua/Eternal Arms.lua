package.path = package.path .. ';./?.lua;'
require("Lua/Invincible Blood")

local function zoneCards(player,zone)
	local cards = {}
	for i=0,getZoneSize(player,zone)-1 do cards[#cards+1] = getCardAt(player,zone,i) end
	return cards
end

local function topCard(player)
	local size = getZoneSize(player,ZONE_DECK)
	if(size==0) then return -1 end
	return getCardAt(player,ZONE_DECK,size-1)
end

local function moveTop(player,zone)
	local card = topCard(player)
	if(card>=0) then moveCard(card,zone) end
end

local dualCivilizations = {
	["Deklowaz, the Terminator"]={CIV_DARKNESS,CIV_FIRE},
	["Bluum Erkis, Flare Guardian"]={CIV_LIGHT,CIV_WATER},
	["Tanzanyte, the Awakener"]={CIV_WATER,CIV_DARKNESS},
	["Bombazar, Dragon of Destiny"]={CIV_FIRE,CIV_NATURE},
	["Techno Totem"]={CIV_LIGHT,CIV_NATURE},
	["Aqua Skydiver"]={CIV_LIGHT,CIV_WATER},
	["Estol, Vizier of Aqua"]={CIV_LIGHT,CIV_WATER},
	["Tajimal, Vizier of Aqua"]={CIV_LIGHT,CIV_WATER},
	["Melnia, the Aqua Shadow"]={CIV_WATER,CIV_DARKNESS},
	["Pointa, the Aqua Shadow"]={CIV_WATER,CIV_DARKNESS},
	["Soderlight, the Cold Blade"]={CIV_WATER,CIV_DARKNESS},
	["Dolmarks, the Shadow Warrior"]={CIV_DARKNESS,CIV_FIRE},
	["Galek, the Shadow Warrior"]={CIV_DARKNESS,CIV_FIRE},
	["Ulex, the Dauntless"]={CIV_DARKNESS,CIV_FIRE},
	["Gonta, the Warrior Savage"]={CIV_FIRE,CIV_NATURE},
	["Tagtapp, the Retaliator"]={CIV_FIRE,CIV_NATURE},
	["Wind Axe, the Warrior Savage"]={CIV_FIRE,CIV_NATURE},
	["Lukia Lex, Pinnacle Guardian"]={CIV_LIGHT,CIV_NATURE},
	["Sanfist, the Savage Vizier"]={CIV_LIGHT,CIV_NATURE},
	["Skysword, the Savage Vizier"]={CIV_LIGHT,CIV_NATURE}
}

local function hasCivilization(card,civ)
	if(getCardCiv(card)==civ) then return true end
	local colors = dualCivilizations[getCardName(card)]
	if(colors~=nil) then
		for _,color in ipairs(colors) do if(color==civ) then return true end end
	end
	return false
end

local silentNames = {
	["Kejila, the Hidden Horror"]=true,["Bulgluf, the Spydroid"]=true,
	["Flohdani, the Spydroid"]=true,["Kaemira, the Oracle"]=true,
	["Charge Whipper"]=true,["Milporo"]=true,["Pinpoint Lunatron"]=true,
	["Gigamente"]=true,["Venom Capsule"]=true,["Brad, Super Kickin' Dynamo"]=true,
	["Minelord Skyterror"]=true,["Vorg's Engine"]=true,["Hustle Berry"]=true,
	["Sporeblast Erengi"]=true,["Soderlight, the Cold Blade"]=true
}

local function entersManaTapped(id)
	if(getMessageType()=="post cardmove" and getMessageInt("card")==id and getMessageInt("to")==ZONE_MANA) then tapCard(id) end
end

local function silentSkill(id,ability) --test
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

local function forcedBlocker(id) --test
	if(getMessageType()=="get creatureforcedblocker" and getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==0 and getCardOwner(id)~=getCardOwner(getMessageInt("attacker")) and getMessageInt("forcedblocker")<0) then
		setMessageInt("forcedblocker",id)
	end
end

local function destroyByPower(id,maximum)
	local cards = zoneCards(0,ZONE_BATTLE)
	for _,card in ipairs(zoneCards(1,ZONE_BATTLE)) do cards[#cards+1]=card end
	for _,card in ipairs(cards) do if(getCreaturePower(card)<=maximum) then destroyCreature(card) end end
end

local function chooseShieldToHandWithTrigger(id) --test
	local owner = getCardOwner(id)
	local shield = createChoice("Choose one of your shields",0,id,owner,Checks.InYourShields)
	if(shield>=0) then
		moveCard(shield,ZONE_HAND)
		if(getCardIsShieldTrigger(shield)==1) then
			local use = createChoiceNoCheck("Use this shield trigger?",2,id,owner,Checks.False)
			if(use==RETURN_BUTTON1) then moveCard(shield,ZONE_BATTLE) end
		end
	end
end

local function buffUntilEnd(source,target,ability)
	local mod = function(cid,mid)
		ability(cid)
		if(getMessageType()=="pre endturn") then destroyModifier(cid,mid) end
	end
	createModifier(target,mod)
end

local function addPowerForTappedMana(id,multiplier,player)
	local count = 0
	for _,card in ipairs(zoneCards(player,ZONE_MANA)) do if(isCardTapped(card)==1) then count=count+1 end end
	Abils.bonusPower(id,count*multiplier)
end

local function searchDeck(id,check,prompt)
	local owner = getCardOwner(id)
	local card = createChoice(prompt,1,id,owner,check)
	if(card>=0) then moveCard(card,ZONE_HAND) end
	shuffleDeck(owner)
end

Cards["Deklowaz, the Terminator"] = {shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	entersManaTapped(id)
	Abils.TapAbility(id,function(id)
		destroyByPower(id,3000)
		local opponent=getOpponent(getCardOwner(id))
		for _,card in ipairs(zoneCards(opponent,ZONE_HAND)) do if(getCardType(card)==TYPE_CREATURE and getCreaturePower(card)<=3000) then discardCard(card) end end
	end)
end}

Cards["Bluum Erkis, Flare Guardian"] = {shieldtrigger=0,blocker=0,breaker=2,HandleMessage=function(id) --test
	entersManaTapped(id)
	if(getMessageType()=="mod breakshield" and getMessageInt("attacker")==id) then
		local shield=getMessageInt("shield")
		setMessageInt("msgContinue",0)
		moveCard(shield,ZONE_HAND)
		if(getCardType(shield)==TYPE_SPELL and getCardIsShieldTrigger(shield)==1) then moveCard(shield,ZONE_BATTLE) end
	end
end}

Cards["Hawkeye Lunatron"] = {shieldtrigger=0,blocker=0,breaker=2,HandleMessage=function(id)
	Abils.onSummon(id,function(id) searchDeck(id,Checks.InYourDeck,"Choose a card from your deck") end)
end}

Cards["Bodacious Giant"] = {shieldtrigger=0,blocker=0,breaker=2,HandleMessage=function(id) --test
	local owner=getCardOwner(id)
	if(getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==1 and getTurn()~=owner) then
		for _,card in ipairs(zoneCards(getOpponent(owner),ZONE_BATTLE)) do Abils.attacksEachTurn(card) end
	end
end}

Cards["Terradragon Dakma Balgarow"] = {shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id)
	Abils.bonusPower(id,(getZoneSize(0,ZONE_SHIELD)+getZoneSize(1,ZONE_SHIELD))*2000)
	if(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id) then
		local power=getCreaturePower(id)
		if(power>=15000) then setMessageInt("breaker",3) elseif(power>=6000 and getMessageInt("breaker")<2) then setMessageInt("breaker",2) end
	end
end}

Cards["Elixia, Pureblade Elemental"] = {shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id)
	local seen,count={},0
	for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_MANA)) do
		for civ=CIV_LIGHT,CIV_DARKNESS do if(hasCivilization(card,civ) and not seen[civ]) then seen[civ]=true count=count+1 end end
	end
	Abils.bonusPower(id,count*3000)
	if(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id) then
		local power=getCreaturePower(id)
		if(power>=15000) then setMessageInt("breaker",3) elseif(power>=6000 and getMessageInt("breaker")<2) then setMessageInt("breaker",2) end
	end
end}

Cards["Ultimate Dragon"] = {shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id)
	local dragons=0
	for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do if(card~=id and isCreatureOfRace(card,"Dragon")==1) then dragons=dragons+1 end end
	Abils.bonusPower(id,dragons*5000)
	Abils.Breaker(id,dragons+1)
end}

Cards["Core-Crash Lizard"] = {shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id)
	Abils.onSummon(id,function(id) local ch=createChoice("Choose an opponent's shield",0,id,getCardOwner(id),Checks.InOppShields) if(ch>=0) then moveCard(ch,ZONE_GRAVEYARD) end end)
end}

Cards["Hurricane Crawler"] = {shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	Abils.onSummon(id,function(id)
		local owner=getCardOwner(id) local hand=zoneCards(owner,ZONE_HAND)
		for _,card in ipairs(hand) do moveCard(card,ZONE_MANA) end
		for i=1,#hand do local ch=createChoice("Choose a card from your mana zone",0,id,owner,Checks.InYourMana) if(ch>=0) then moveCard(ch,ZONE_HAND) end end
	end)
end}

Cards["Necrodragon Bryzenaga"] = {shieldtrigger=0,blocker=0,breaker=2,HandleMessage=function(id) --test
	Abils.onSummon(id,function(id)
		local owner=getCardOwner(id)
		for _,shield in ipairs(zoneCards(owner,ZONE_SHIELD)) do
			moveCard(shield,ZONE_HAND)
			if(getCardIsShieldTrigger(shield)==1) then local use=createChoiceNoCheck("Use this shield trigger?",2,id,owner,Checks.False) if(use==RETURN_BUTTON1) then moveCard(shield,ZONE_BATTLE) end end
		end
	end)
end}

Cards["Balza, Seeker of Hyperpearls"] = {shieldtrigger=1,blocker=1,breaker=1,HandleMessage=function(id) Abils.Blocker(id) end}
Cards["Ryudmila, Channeler of Suns"] = {shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	local count=0 for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do if(card~=id and isCardTapped(card)==0) then count=count+1 end end
	Abils.bonusPower(id,count*2000)
	if(getMessageType()=="mod creaturedestroy" and getMessageInt("creature")==id) then setMessageInt("zoneto",ZONE_DECK) shuffleDeck(getCardOwner(id)) end
end}
Cards["King Oquanos"] = {shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id)
	addPowerForTappedMana(id,2000,getOpponent(getCardOwner(id)))
	if(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id and getCreaturePower(id)>=6000) then Abils.Breaker(id,2) end
end}

Cards["Gajirabute, Vile Centurion"] = {shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id)
	Abils.onSummon(id,function(id) local ch=createChoice("Choose an opponent's shield to put into their graveyard",0,id,getCardOwner(id),Checks.InOppShields) if(ch>=0) then moveCard(ch,ZONE_GRAVEYARD) end end)
end}

Cards["Kejila, the Hidden Horror"] = {shieldtrigger=0,blocker=0,breaker=2,HandleMessage=function(id) --test
	silentSkill(id,function(id) for i=1,2 do local ch=createChoice("Choose an opponent's shield to break",0,id,getCardOwner(id),Checks.InOppShields) if(ch>=0) then creatureBreakShield(id,ch) end end end)
end}
Cards["Gaulezal Dragon"] = {shieldtrigger=0,blocker=0,breaker=2,HandleMessage=function(id) end}
Cards["Carnival Totem"] = {shieldtrigger=0,blocker=0,breaker=2,HandleMessage=function(id) --test
	Abils.onSummon(id,function(id)
		local owner=getCardOwner(id) local mana=zoneCards(owner,ZONE_MANA) local hand=zoneCards(owner,ZONE_HAND)
		for _,card in ipairs(mana) do moveCard(card,ZONE_HAND) end
		for _,card in ipairs(hand) do moveCard(card,ZONE_MANA) tapCard(card) end
	end)
end}
Cards["Tanzanyte, the Awakener"] = {shieldtrigger=0,blocker=0,breaker=2,HandleMessage=function(id) --test
	entersManaTapped(id)
	Abils.TapAbility(id,function(id)
		local chosen=createChoice("Choose a creature in your graveyard",0,id,getCardOwner(id),Checks.CreatureInYourGraveyard)
		if(chosen>=0) then local name=getCardName(chosen) for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_GRAVEYARD)) do if(getCardType(card)==TYPE_CREATURE and getCardName(card)==name) then moveCard(card,ZONE_HAND) end end end
	end)
end}
Cards["Bombazar, Dragon of Destiny"] = {shieldtrigger=0,blocker=0,breaker=2,HandleMessage=function(id) --test
	entersManaTapped(id) Abils.SpeedAttacker(id)
	Abils.onSummon(id,function(id)
		for player=0,1 do for _,card in ipairs(zoneCards(player,ZONE_BATTLE)) do if(card~=id and getCreaturePower(card)==6000) then destroyCreature(card) end end end
		local owner=getCardOwner(id) local extra=false
		local mod=function(cid,mid)
			if(getMessageType()=="mod endturn" and getMessageInt("player")==owner) then
				if(not extra) then setMessageInt("extraturn",1) extra=true else loseGame(owner) destroyModifier(cid,mid) end
			end
		end
		createModifier(id,mod)
	end)
end}
Cards["Techno Totem"] = {shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	entersManaTapped(id)
	if(getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==1) then for _,card in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do if(card~=id) then Abils.PowerAttacker(card,1500) end end end
	Abils.TapAbility(id,function(id) local ch=createChoice("Choose an opponent's creature to tap",0,id,getCardOwner(id),Checks.InOppBattle) if(ch>=0) then tapCard(ch) end end)
end}

Cards["Berochika, Channeler of Suns"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) Abils.onSummon(id,function(id) local p=getCardOwner(id) if(getZoneSize(p,ZONE_SHIELD)>=5) then moveTop(p,ZONE_SHIELD) end end) end}
Cards["Bulgluf, the Spydroid"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	silentSkill(id,function(id) moveTop(getCardOwner(id),ZONE_SHIELD) end)
end}
Cards["Clearlo, Grace Enforcer"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) local n=0 for _,c in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do if(c~=id and isCardTapped(c)==0) then n=n+1 end end Abils.bonusPower(id,n*1000) end}
Cards["Ferrosaturn, Spectral Knight"]={shieldtrigger=0,blocker=1,breaker=1,HandleMessage=function(id) Abils.Blocker(id) Abils.cantAttackPlayers(id) end}
Cards["Flohdani, the Spydroid"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	silentSkill(id,function(id) for i=1,2 do local ch=createChoice("Choose an opponent's creature to tap",1,id,getCardOwner(id),Checks.InOppBattle) if(ch>=0) then tapCard(ch) else break end end end)
end}
Cards["Glais Mejicula, the Extreme"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	Abils.Evolution(id,"Initiate")
	if(getMessageType()=="mod breakshield" and getCardZone(id)==ZONE_BATTLE and getCardOwner(getMessageInt("shield"))==getCardOwner(id) and getZoneSize(getCardOwner(id),ZONE_HAND)>=2) then
		local use=createChoiceNoCheck("Discard 2 cards instead of breaking this shield?",2,id,getCardOwner(id),Checks.False)
		if(use==RETURN_BUTTON1) then for i=1,2 do local ch=createChoice("Choose a card to discard",0,id,getCardOwner(id),Checks.InYourHand) if(ch>=0) then discardCard(ch) end end setMessageInt("msgContinue",0) end
	end
end}
Cards["Ikaz, the Spydroid"]={shieldtrigger=0,blocker=1,breaker=1,HandleMessage=function(id) --test
	Abils.Blocker(id) Abils.cantAttackPlayers(id)
	if(getMessageType()=="post creaturebattle" and getMessageInt("blocked")==1 and getMessageInt("defender")==id) then local ch=createChoice("Choose one of your creatures to untap",0,id,getCardOwner(id),Checks.InYourBattle) if(ch>=0) then untapCard(ch) end end
end}
Cards["Kaemira, the Oracle"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	silentSkill(id,function(id) moveTop(getCardOwner(id),ZONE_SHIELD) end)
end}
Cards["Lemik, Vizier of Thought"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) if(getCardZone(id)==ZONE_BATTLE) then for _,c in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do if(hasCivilization(c,CIV_WATER) or hasCivilization(c,CIV_NATURE)) then Abils.Blocker(c) end end end end}
Cards["Logic Cube"]={shieldtrigger=1,OnCast=function(id) searchDeck(id,Checks.SpellInYourDeck,"Choose a spell from your deck") end}
Cards["Messa Bahna, Expanse Guardian"]={shieldtrigger=0,blocker=1,breaker=1,HandleMessage=function(id) --test
	Abils.Blocker(id) Abils.cantAttackPlayers(id) forcedBlocker(id)
end}
Cards["Pala Olesis, Morning Guardian"]={shieldtrigger=0,blocker=1,breaker=1,HandleMessage=function(id) Abils.Blocker(id) Abils.cantAttackPlayers(id) if(getCardZone(id)==ZONE_BATTLE and getTurn()~=getCardOwner(id)) then for _,c in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do if(c~=id) then Abils.bonusPower(c,2000) end end end end}
Cards["Poltalester, the Spydroid"]={shieldtrigger=1,blocker=1,breaker=1,HandleMessage=function(id) Abils.Blocker(id) end}
Cards["Rapid Reincarnation"]={shieldtrigger=1,OnCast=function(id) --test
	local owner=getCardOwner(id) local victim=createChoice("Choose one of your creatures to destroy",1,id,owner,Checks.InYourBattle)
	if(victim>=0) then destroyCreature(victim) local valid=function(cid,sid) if(Checks.InYourHand(cid,sid)==1 and getCardType(sid)==TYPE_CREATURE and getCardCost(sid)<=getZoneSize(owner,ZONE_MANA)) then return 1 end return 0 end local ch=createChoice("Choose a creature to put into battle",0,id,owner,valid) if(ch>=0) then moveCard(ch,ZONE_BATTLE) end end
end}
Cards["Solar Ray"]={shieldtrigger=1,OnCast=function(id) local ch=createChoice("Choose an opponent's creature to tap",0,id,getCardOwner(id),Checks.InOppBattle) if(ch>=0) then tapCard(ch) end end}
Cards["Static Warp"]={shieldtrigger=0,OnCast=function(id) --test
	local keep={} for player=0,1 do local valid=function(cid,sid) if(Checks.InBattle(cid,sid)==1 and getCardOwner(sid)==player) then return 1 end return 0 end local ch=createChoice("Choose a creature to remain untapped",0,id,player,valid) if(ch>=0) then keep[ch]=true end end
	for player=0,1 do for _,c in ipairs(zoneCards(player,ZONE_BATTLE)) do if(not keep[c]) then tapCard(c) end end end
end}
Cards["Tulk, the Oracle"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) end}

Cards["Aqua Strummer"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	Abils.onSummon(id,function(id)
		local owner=getCardOwner(id) local size=getZoneSize(owner,ZONE_DECK) local count=math.min(5,size) local available={}
		for i=1,count do local c=getCardAt(owner,ZONE_DECK,size-i) available[c]=true unflipCard(c) setCardVisibility(c,owner,1) end
		local valid=function(cid,sid) if(available[sid]) then return 1 end return 0 end
		for i=1,count do local ch=createChoice("Choose the next card to put on top",0,id,owner,valid) if(ch>=0) then available[ch]=nil moveCard(ch,ZONE_DECK) end end
		for c in pairs(available) do flipCard(c) setCardVisibility(c,owner,0) end
	end)
end}
Cards["Ardent Lunatron"]={shieldtrigger=0,blocker=1,breaker=1,HandleMessage=function(id) --test
	Abils.Blocker(id) Abils.cantAttack(id) forcedBlocker(id)
end}
Cards["Battery Cluster"]={shieldtrigger=0,blocker=1,breaker=1,HandleMessage=function(id) Abils.Blocker(id) Abils.cantAttack(id) end}
Cards["Buoyant Blowfish"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) addPowerForTappedMana(id,1000,getOpponent(getCardOwner(id))) end}
Cards["Charge Whipper"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	silentSkill(id,function(id) local owner=getCardOwner(id) local card=createChoice("Choose a card from your hand to add to shields",1,id,owner,Checks.InYourHand) if(card>=0) then moveCard(card,ZONE_SHIELD) local shield=createChoice("Choose one of your shields to put into your hand",0,id,owner,Checks.InYourShields) if(shield>=0) then moveCard(shield,ZONE_HAND) end end end)
end}
Cards["Crystal Spinslicer"]={shieldtrigger=0,blocker=1,breaker=1,HandleMessage=function(id) Abils.Blocker(id) Abils.Evolution(id,"Liquid People") end}
Cards["Fluorogill Manta"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) if(getCardZone(id)==ZONE_BATTLE) then for _,c in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do if(hasCivilization(c,CIV_LIGHT) or hasCivilization(c,CIV_DARKNESS)) then Abils.cantBeBlocked(c) end end end end}
Cards["Milporo"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	silentSkill(id,function(id) drawCards(getCardOwner(id),1) end)
end}
Cards["Mystic Magician"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post cardmove" and getMessageInt("to")==ZONE_BATTLE and silentNames[getCardName(getMessageInt("card"))]) then tapCard(getMessageInt("card")) end
	if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="mod creaturedestroy" and getCardOwner(getMessageInt("creature"))==getCardOwner(id) and silentNames[getCardName(getMessageInt("creature"))]) then setMessageInt("zoneto",ZONE_HAND) end
end}
Cards["Pinpoint Lunatron"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	silentSkill(id,function(id) local valid=function(cid,sid) if(Checks.InBattle(cid,sid)==1 or getCardZone(sid)==ZONE_MANA) then return 1 end return 0 end local ch=createChoice("Choose a creature or mana card to return",0,id,getCardOwner(id),valid) if(ch>=0) then moveCard(ch,ZONE_HAND) end end)
end}
Cards["Recon Operation"]={shieldtrigger=0,OnCast=function(id) --test
	local seen={} local valid=function(cid,sid) if(Checks.InOppShields(cid,sid)==1 and not seen[sid]) then return 1 end return 0 end
	for i=1,3 do local ch=createChoice("Choose an opponent's shield to look at",1,id,getCardOwner(id),valid) if(ch<0) then break end seen[ch]=true unflipCard(ch) setCardVisibility(ch,getCardOwner(id),1) end
	for c in pairs(seen) do flipCard(c) setCardVisibility(c,getCardOwner(id),0) end
end}
Cards["Siren Concerto"]={shieldtrigger=1,OnCast=function(id) --test
	local owner=getCardOwner(id) local mana=createChoice("Choose a card from your mana zone",0,id,owner,Checks.InYourMana) if(mana>=0) then moveCard(mana,ZONE_HAND) local hand=createChoice("Choose a card from your hand",0,id,owner,Checks.InYourHand) if(hand>=0) then moveCard(hand,ZONE_MANA) end end
end}
Cards["Spiral Gate"]={shieldtrigger=1,OnCast=function(id) local ch=createChoice("Choose a creature to return to its owner's hand",0,id,getCardOwner(id),Checks.InBattle) if(ch>=0) then moveCard(ch,ZONE_HAND) end end}
Cards["Tide Patroller"]={shieldtrigger=0,blocker=1,breaker=1,HandleMessage=function(id) Abils.Blocker(id) end}
Cards["Torpedo Cluster"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) Abils.onSummon(id,function(id) local ch=createChoice("Choose a card from your mana zone",0,id,getCardOwner(id),Checks.InYourMana) if(ch>=0) then moveCard(ch,ZONE_HAND) end end) end}
Cards["Transmogrify"]={shieldtrigger=1,OnCast=function(id) --test
	local victim=createChoice("Choose a creature to destroy",1,id,getCardOwner(id),Checks.InBattle)
	if(victim>=0) then local owner=getCardOwner(victim) destroyCreature(victim) local deck=zoneCards(owner,ZONE_DECK) for i=#deck,1,-1 do local c=deck[i] if(getCardType(c)==TYPE_CREATURE and getCreatureIsEvolution(c)==0) then moveCard(c,ZONE_BATTLE) break else moveCard(c,ZONE_GRAVEYARD) end end end
end}
Cards["Zaltan"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post cardmove" and getMessageInt("to")==ZONE_BATTLE and getCardOwner(getMessageInt("card"))==getCardOwner(id) and isCreatureOfRace(getMessageInt("card"),"Cyber Virus")==1) then
		for i=1,2 do local discard=createChoice("Choose a card to discard",1,id,getCardOwner(id),Checks.InYourHand) if(discard<0) then break end discardCard(discard) local target=createChoice("Choose a creature to return",0,id,getCardOwner(id),Checks.InBattle) if(target>=0) then moveCard(target,ZONE_HAND) end end
	end
end}

Cards["Benzo, the Hidden Fury"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	Abils.onSummon(id,chooseShieldToHandWithTrigger)
end}
Cards["Death Smoke"]={shieldtrigger=0,OnCast=function(id) local ch=createChoice("Choose an untapped opponent's creature",0,id,getCardOwner(id),Checks.UntappedInOppBattle) if(ch>=0) then destroyCreature(ch) end end}
Cards["Dedreen, the Hidden Corrupter"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) Abils.onSummon(id,function(id) local opp=getOpponent(getCardOwner(id)) if(getZoneSize(opp,ZONE_SHIELD)<=3) then discardCardAtRandom(opp) end end) end}
Cards["Gigamente"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	silentSkill(id,function(id) local ch=createChoice("Choose a creature from your graveyard",0,id,getCardOwner(id),Checks.CreatureInYourGraveyard) if(ch>=0) then moveCard(ch,ZONE_HAND) end end)
end}
Cards["Gigandura"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	Abils.onSummon(id,function(id) local card=createChoice("Choose a card in your opponent's hand",1,id,getCardOwner(id),Checks.InOppHand) if(card>=0) then moveCard(card,ZONE_MANA) local mana=createChoice("Choose a card in your opponent's mana zone",0,id,getCardOwner(id),Checks.InOppMana) if(mana>=0) then moveCard(mana,ZONE_HAND) end end end)
end}
Cards["Hourglass Mutant"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) if(getCardZone(id)==ZONE_BATTLE) then for _,c in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do if(hasCivilization(c,CIV_WATER) or hasCivilization(c,CIV_FIRE)) then Abils.Slayer(c) end end end end}
Cards["Infernal Command"]={shieldtrigger=1,OnCast=function(id) --test
	local ch=createChoice("Choose an opponent's creature",0,id,getCardOwner(id),Checks.InOppBattle) if(ch>=0) then local owner=getCardOwner(id) local mod=function(cid,mid) Abils.attacksEachTurn(cid) if(getMessageType()=="pre startturn" and getMessageInt("player")==owner) then destroyModifier(cid,mid) end end createModifier(ch,mod) end
end}
Cards["Mikay, Rattling Doll"]={shieldtrigger=0,blocker=1,breaker=1,HandleMessage=function(id) Abils.Blocker(id) Abils.cantAttack(id) end}
Cards["Mummy Wrap, Shadow of Fatigue"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) Abils.TapAbility(id,function(id) discardCardAtRandom(0) discardCardAtRandom(1) end) end}
Cards["Nightmare Invader"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) end}
Cards["Pierr, Psycho Doll"]={shieldtrigger=0,blocker=1,breaker=1,HandleMessage=function(id) --test
	Abils.Blocker(id) Abils.cantAttack(id) Abils.Slayer(id) forcedBlocker(id)
end}
Cards["Spark Chemist, Shadow of Whim"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) Abils.onSummon(id,function(id) for _,c in ipairs(zoneCards(getCardOwner(id),ZONE_MANA)) do moveCard(c,ZONE_HAND) end end) end}
Cards["Spinal Parasite"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="pre startturn" and getMessageInt("player")~=getCardOwner(id)) then local player=getMessageInt("player") local valid=function(cid,sid) if(getCardOwner(sid)==player and Checks.InBattle(cid,sid)==1 and isCardTapped(sid)==0) then return 1 end return 0 end local ch=createChoice("Choose a creature that must attack this turn",0,id,player,valid) if(ch>=0) then local mod=function(cid,mid) Abils.attacksEachTurn(cid) if(getMessageType()=="pre endturn" and getMessageInt("player")==player) then destroyModifier(cid,mid) end end createModifier(ch,mod) end end
end}
Cards["Uliya, the Entrancer"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	Abils.onSummon(id,chooseShieldToHandWithTrigger)
end}
Cards["Upheaval"]={shieldtrigger=1,OnCast=function(id) --test
	for player=0,1 do local mana=zoneCards(player,ZONE_MANA) local hand=zoneCards(player,ZONE_HAND) for _,c in ipairs(mana) do moveCard(c,ZONE_HAND) end for _,c in ipairs(hand) do moveCard(c,ZONE_MANA) tapCard(c) end end
end}
Cards["Venom Capsule"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	silentSkill(id,function(id) local ch=createChoice("Choose an opponent's shield to break",0,id,getCardOwner(id),Checks.InOppShields) if(ch>=0) then creatureBreakShield(id,ch) end end)
end}
Cards["Zero Nemesis, Shadow of Panic"]={shieldtrigger=0,blocker=0,breaker=2,HandleMessage=function(id)
	Abils.Evolution(id,"Ghost")
	if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post creatureattack" and getCardOwner(getMessageInt("attacker"))==getCardOwner(id)) then discardCardAtRandom(getOpponent(getCardOwner(id))) end
end}

Cards["Armored Raider Gandaval"]={shieldtrigger=0,blocker=0,breaker=2,HandleMessage=function(id) Abils.Evolution(id,"Human") local n=Functions.countTappedCreaturesInBattle(getCardOwner(id)) if(isCardTapped(id)==1) then n=n-1 end Abils.PowerAttacker(id,n*2000) end}
Cards["Brad, Super Kickin' Dynamo"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	silentSkill(id,function(id) local ch=createChoice("Choose an opponent's blocker to destroy",0,id,getCardOwner(id),Checks.BlockerInOppBattle) if(ch>=0) then destroyCreature(ch) end end)
end}
Cards["Burnwisp Lizard"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) if(getCardZone(id)==ZONE_BATTLE) then for _,c in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do if(silentNames[getCardName(c)]) then Abils.SpeedAttacker(c) end end end end}
Cards["Colossus Boost"]={shieldtrigger=0,OnCast=function(id) local ch=createChoice("Choose one of your creatures",0,id,getCardOwner(id),Checks.InYourBattle) if(ch>=0) then buffUntilEnd(id,ch,function(cid) Abils.bonusPower(cid,4000) end) end end}
Cards["Cragsaur"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) end}
Cards["Explosive Trooper Zalmez"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) Abils.onSummon(id,function(id) if(getZoneSize(getOpponent(getCardOwner(id)),ZONE_SHIELD)<=2) then local valid=function(cid,sid) if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=3000) then return 1 end return 0 end local ch=createChoice("Choose an opponent's creature to destroy",1,id,getCardOwner(id),valid) if(ch>=0) then destroyCreature(ch) end end end) end}
Cards["Forced Frenzy"]={shieldtrigger=1,OnCast=function(id) --test
	local owner=getCardOwner(id) for _,c in ipairs(zoneCards(getOpponent(owner),ZONE_BATTLE)) do local mod=function(cid,mid) Abils.attacksEachTurn(cid) if(getMessageType()=="pre startturn" and getMessageInt("player")==owner) then destroyModifier(cid,mid) end end createModifier(c,mod) end
end}
Cards["Hurlosaur"]={shieldtrigger=1,blocker=0,breaker=1,HandleMessage=function(id) Abils.onSummon(id,function(id) local valid=function(cid,sid) if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=1000) then return 1 end return 0 end local ch=createChoice("Choose an opponent's creature to destroy",0,id,getCardOwner(id),valid) if(ch>=0) then destroyCreature(ch) end end) end}
Cards["Mezger, Commando Leader"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) Abils.SpeedAttacker(id) end}
Cards["Minelord Skyterror"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	silentSkill(id,function(id) destroyByPower(id,3000) end)
end}
Cards["Mykee's Pliers"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) if(getCardZone(id)==ZONE_BATTLE) then for _,c in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do if(hasCivilization(c,CIV_DARKNESS) or hasCivilization(c,CIV_NATURE)) then Abils.SpeedAttacker(c) end end end end}
Cards["Phantom Dragon's Flame"]={shieldtrigger=1,OnCast=function(id) local valid=function(cid,sid) if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=2000) then return 1 end return 0 end local ch=createChoice("Choose an opponent's creature to destroy",0,id,getCardOwner(id),valid) if(ch>=0) then destroyCreature(ch) end end}
Cards["Siege Roller Bagash"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) local n=Functions.countTappedCreaturesInBattle(getCardOwner(id)) if(isCardTapped(id)==1) then n=n-1 end Abils.PowerAttacker(id,n*1000) end}
Cards["Smash Warrior Stagrandu"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) Abils.canAttackUntappedCreatures(id) if(getMessageType()=="post creatureattack" and getMessageInt("attacker")==id and getMessageInt("defendertype")==DEFENDER_CREATURE and getCreaturePower(getMessageInt("defender"))>=6000) then buffUntilEnd(id,id,function(cid) Abils.bonusPower(cid,9000) end) end end}
Cards["Supersonic Jet Pack"]={shieldtrigger=0,OnCast=function(id) local ch=createChoice("Choose one of your creatures",0,id,getCardOwner(id),Checks.InYourBattle) if(ch>=0) then buffUntilEnd(id,ch,function(cid) Abils.SpeedAttacker(cid) end) end end}
Cards["Taunting Skyterror"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	if(getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==1 and getTurn()~=getCardOwner(id)) then for _,c in ipairs(zoneCards(getOpponent(getCardOwner(id)),ZONE_BATTLE)) do Abils.attacksEachTurn(c) end end
end}
Cards["Vorg's Engine"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	silentSkill(id,function(id) destroyByPower(id,2000) end)
end}

Cards["Adventure Boar"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) Abils.PowerAttacker(id,2000) end}
Cards["Ancient Horn, the Watcher"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) Abils.onSummon(id,function(id) local owner=getCardOwner(id) if(getZoneSize(owner,ZONE_SHIELD)>=5) then for _,c in ipairs(zoneCards(owner,ZONE_MANA)) do untapCard(c) end end end) end}
Cards["Bubble Scarab"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="post creatureattack" and getMessageInt("defendertype")==DEFENDER_CREATURE and getCardOwner(getMessageInt("defender"))==getCardOwner(id)) then local card=createChoice("Choose a card to discard",1,id,getCardOwner(id),Checks.InYourHand) if(card>=0) then discardCard(card) buffUntilEnd(id,getMessageInt("defender"),function(cid) Abils.bonusPower(cid,3000) end) end end
end}
Cards["Earth Ripper, Talon of Rage"]={shieldtrigger=0,blocker=0,breaker=2,HandleMessage=function(id) Abils.Evolution(id,"Beast Folk") Abils.onSummon(id,function(id) for _,c in ipairs(zoneCards(getCardOwner(id),ZONE_MANA)) do if(isCardTapped(c)==1) then moveCard(c,ZONE_HAND) end end end) end}
Cards["Faerie Life"]={shieldtrigger=1,OnCast=function(id) moveTop(getCardOwner(id),ZONE_MANA) end}
Cards["Hustle Berry"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	silentSkill(id,function(id) moveTop(getCardOwner(id),ZONE_MANA) end)
end}
Cards["Jiggly Totem"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) addPowerForTappedMana(id,1000,getCardOwner(id)) end}
Cards["Karate Potato"]={shieldtrigger=1,blocker=0,breaker=1,HandleMessage=function(id) Abils.onSummon(id,function(id) for i=1,2 do local ch=createChoice("Choose a card from your hand to put into mana",1,id,getCardOwner(id),Checks.InYourHand) if(ch>=0) then moveCard(ch,ZONE_MANA) else break end end end) end}
Cards["Legacy Shell"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) if(getCardZone(id)==ZONE_BATTLE) then for _,c in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do if(hasCivilization(c,CIV_LIGHT) or hasCivilization(c,CIV_FIRE)) then Abils.PowerAttacker(c,3000) end end end end}
Cards["Sabermask Scarab"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) Abils.onAttack(id,function(id) local ch=createChoice("Choose a card from your mana zone",0,id,getCardOwner(id),Checks.InYourMana) if(ch>=0) then moveCard(ch,ZONE_HAND) end end) end}
Cards["Scowling Tomato"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) end}
Cards["Shaman Broccoli"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) Abils.manaAfterDestroyed(id) end}
Cards["Soulswap"]={shieldtrigger=1,OnCast=function(id) --test
	local victim=createChoice("Choose a creature to put into its owner's mana zone",1,id,getCardOwner(id),Checks.InBattle)
	if(victim>=0) then local player=getCardOwner(victim) moveCard(victim,ZONE_MANA) local valid=function(cid,sid) if(getCardOwner(sid)==player and getCardZone(sid)==ZONE_MANA and getCardType(sid)==TYPE_CREATURE and getCreatureIsEvolution(sid)==0 and getCardCost(sid)<=getZoneSize(player,ZONE_MANA)) then return 1 end return 0 end local ch=createChoice("Choose a non-evolution creature from that mana zone",0,id,player,valid) if(ch>=0) then moveCard(ch,ZONE_BATTLE) end end
end}
Cards["Sporeblast Erengi"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	silentSkill(id,function(id) searchDeck(id,Checks.CreatureInYourDeck,"Choose a creature from your deck") end)
end}
Cards["Terradragon Cusdalf"]={shieldtrigger=0,blocker=0,breaker=2,HandleMessage=function(id) --test
	Abils.PowerAttacker(id,4000)
	if(getCardZone(id)==ZONE_BATTLE and getMessageType()=="mod carduntap" and getCardOwner(getMessageInt("card"))==getCardOwner(id) and getCardZone(getMessageInt("card"))==ZONE_MANA and getTurn()==getCardOwner(id)) then setMessageInt("msgContinue",0) end
end}
Cards["Thirst for the Hunt"]={shieldtrigger=0,OnCast=function(id) for _,c in ipairs(zoneCards(getCardOwner(id),ZONE_BATTLE)) do buffUntilEnd(id,c,function(cid) Abils.PowerAttacker(cid,1000) end) end end}
Cards["Twitch Horn, the Aggressor"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) addPowerForTappedMana(id,2000,getCardOwner(id)) end}

Cards["Aqua Skydiver"]={shieldtrigger=1,blocker=1,breaker=1,HandleMessage=function(id) --test
	entersManaTapped(id) Abils.Blocker(id) Abils.returnAfterDestroyed(id)
end}
Cards["Estol, Vizier of Aqua"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	entersManaTapped(id) Abils.onSummon(id,function(id) moveTop(getCardOwner(id),ZONE_SHIELD) local ch=createChoice("Choose an opponent's shield to look at",0,id,getCardOwner(id),Checks.InOppShields) if(ch>=0) then unflipCard(ch) setCardVisibility(ch,getCardOwner(id),1) flipCard(ch) setCardVisibility(ch,getCardOwner(id),0) end end)
end}
Cards["Tajimal, Vizier of Aqua"]={shieldtrigger=0,blocker=1,breaker=1,HandleMessage=function(id) --test
	entersManaTapped(id) Abils.Blocker(id) Abils.cantAttackPlayers(id)
	local battlingFire = (getAttacker()==id and getDefenderType()==DEFENDER_CREATURE and hasCivilization(getDefender(),CIV_FIRE)) or (getDefender()==id and hasCivilization(getAttacker(),CIV_FIRE))
	if(getMessageType()=="get creaturepower" and getMessageInt("creature")==id and battlingFire) then setMessageInt("power",getMessageInt("power")+4000) end
end}
Cards["Melnia, the Aqua Shadow"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) entersManaTapped(id) Abils.cantBeBlocked(id) Abils.Slayer(id) end}
Cards["Pointa, the Aqua Shadow"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	entersManaTapped(id) Abils.onSummon(id,function(id) local ch=createChoice("Choose an opponent's shield to look at",0,id,getCardOwner(id),Checks.InOppShields) if(ch>=0) then unflipCard(ch) setCardVisibility(ch,getCardOwner(id),1) flipCard(ch) setCardVisibility(ch,getCardOwner(id),0) end discardCardAtRandom(getOpponent(getCardOwner(id))) end)
end}
Cards["Soderlight, the Cold Blade"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	entersManaTapped(id) Abils.cantBeBlocked(id)
	silentSkill(id,function(id) local opponent=getOpponent(getCardOwner(id)) local ch=createChoice("Choose one of your creatures to destroy",0,id,opponent,Checks.InOppBattle) if(ch>=0) then destroyCreature(ch) end end)
end}
Cards["Dolmarks, the Shadow Warrior"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	entersManaTapped(id) Abils.onSummon(id,function(id)
		local owner=getCardOwner(id) local ch=createChoice("Choose one of your creatures to destroy",0,id,owner,Checks.InYourBattle) if(ch>=0) then destroyCreature(ch) end local mana=createChoice("Choose one of your mana cards to destroy",0,id,owner,Checks.InYourMana) if(mana>=0) then destroyMana(mana) end
		local opp=getOpponent(owner) local creature=createChoice("Choose one of your creatures to destroy",0,id,opp,Checks.InOppBattle) if(creature>=0) then destroyCreature(creature) end local oppmana=createChoice("Choose one of your mana cards to destroy",0,id,opp,Checks.InOppMana) if(oppmana>=0) then destroyMana(oppmana) end
	end)
end}
Cards["Galek, the Shadow Warrior"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) entersManaTapped(id) Abils.onSummon(id,function(id) local ch=createChoice("Choose an opponent's blocker to destroy",0,id,getCardOwner(id),Checks.BlockerInOppBattle) if(ch>=0) then destroyCreature(ch) end discardCardAtRandom(getOpponent(getCardOwner(id))) end) end}
Cards["Ulex, the Dauntless"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) --test
	entersManaTapped(id) if(getMessageType()=="mod cardtap" and getMessageInt("card")==id and getTurn()~=getCardOwner(id)) then setMessageInt("msgContinue",0) end
end}
Cards["Gonta, the Warrior Savage"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) entersManaTapped(id) end}
Cards["Tagtapp, the Retaliator"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) entersManaTapped(id) local n=0 for _,c in ipairs(zoneCards(getOpponent(getCardOwner(id)),ZONE_MANA)) do if(hasCivilization(c,CIV_WATER)) then n=n+1 end end Abils.bonusPower(id,n*1000) if(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id and getCreaturePower(id)>=6000) then Abils.Breaker(id,2) end end}
Cards["Wind Axe, the Warrior Savage"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) entersManaTapped(id) Abils.onSummon(id,function(id) local ch=createChoice("Choose an opponent's blocker to destroy",0,id,getCardOwner(id),Checks.BlockerInOppBattle) if(ch>=0) then destroyCreature(ch) end moveTop(getCardOwner(id),ZONE_MANA) end) end}
Cards["Lukia Lex, Pinnacle Guardian"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) entersManaTapped(id) Abils.PowerAttacker(id,3000) Abils.untapAtEOT(id) end}
Cards["Sanfist, the Savage Vizier"]={shieldtrigger=0,blocker=1,breaker=1,HandleMessage=function(id) --test
	entersManaTapped(id) Abils.Blocker(id)
	if(getMessageType()=="mod carddiscard" and getMessageInt("card")==id and getTurn()~=getCardOwner(id)) then local use=createChoiceNoCheck("Put Sanfist into the battle zone instead?",2,id,getCardOwner(id),Checks.False) if(use==RETURN_BUTTON1) then setMessageInt("zoneto",ZONE_BATTLE) end end
end}
Cards["Skysword, the Savage Vizier"]={shieldtrigger=0,blocker=0,breaker=1,HandleMessage=function(id) entersManaTapped(id) Abils.onSummon(id,function(id) local owner=getCardOwner(id) moveTop(owner,ZONE_MANA) moveTop(owner,ZONE_SHIELD) end) end}
