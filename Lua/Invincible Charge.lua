package.path = package.path .. ';./?.lua;'
require("Lua/Invincible Soul")

Cards["Siri, Glory Elemental"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		if(getMessageType()=="get creatureisblocker") then
			if(getMessageInt("creature")==id) then
				local c = getZoneSize(getCardOwner(id), ZONE_SHIELD)
				if(c<=0) then
					setMessageInt("isblocker",1)
				end
			end
		end

		if(getMessageType()=="pre endturn") then
			if(getMessageInt("player")==getCardOwner(id) and getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==1) then
				local c = getZoneSize(getCardOwner(id), ZONE_SHIELD)
				if(c<=0) then
					local ch = createChoiceNoCheck("Untap this creature?",2,id,getCardOwner(id),Checks.False)
					if(ch==RETURN_BUTTON1) then
						untapCard(id)
					end
				end
			end
		end
	end
}

Cards["Cosmic Nebula"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.Evolution(id,"Cyber Virus")

		if(getMessageType()=="post startturn") then
			if(getMessageInt("player")==getCardOwner(id) and getCardZone(id)==ZONE_BATTLE) then
				local ch = createChoiceNoCheck("Draw extra card?",2,id,getCardOwner(id),Checks.False)
				if(ch==RETURN_BUTTON1) then
					drawCards(getCardOwner(id), 1)
				end
			end
		end
	end
}

Cards["Crath Lade, Merciless King"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			discardCardAtRandom(getOpponent(getCardOwner(id)))
			discardCardAtRandom(getOpponent(getCardOwner(id)))
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Sky Crusher, the Agitator"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local ch = createChoice("Choose a card in your manazone", 0, id, getCardOwner(id), Checks.InYourMana)
			if(ch>=0) then
				moveCard(ch, ZONE_GRAVEYARD)
			end

			local ch2 = createChoice("Choose a card in your manazone", 0, id, getOpponent(getCardOwner(id)), Checks.InOppMana)
			if(ch2>=0) then
				moveCard(ch2, ZONE_GRAVEYARD)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Headlong Giant"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 3,

	HandleMessage = function(id) --test
		Abils.cantBeBlockedPower(id, 4000)

		local func = function(id)
			discardCardAtRandom(getCardOwner(id))
		end
		Abils.onAttack(id,func)
		
		if(getMessageType()=="get creaturecanattackcreature") then
			if(getMessageInt("attacker")==id) then
				local s = getZoneSize(getCardOwner(id), ZONE_HAND)
				if(s==0) then
					setMessageInt("canattack",CANATTACK_NO)
				end
			end
		end
		if(getMessageType()=="get creaturecanattackplayers") then
			if(getMessageInt("attacker")==id) then
				local s = getZoneSize(getCardOwner(id), ZONE_HAND)
				if(s==0) then
					setMessageInt("canattack",CANATTACK_NO)
				end
			end
		end
	end
}

Cards["Gandar, Seeker of Explosions"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		local tap = function(id)
			local owner = getCardOwner(id)
			local mod = function(cid,mid)
				if(getMessageType()=="pre endturn") then
					local size = getZoneSize(owner,ZONE_BATTLE)
					for i=0,(size-1) do
						local creature = getCardAt(owner,ZONE_BATTLE,i)
						if(cardHasCivilization(creature,CIV_LIGHT)) then
							untapCard(creature)
						end
					end
					destroyModifier(cid,mid)
				end
			end
			createModifier(id,mod)
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["King Benthos"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		local tap = function(id)
			local owner = getCardOwner(id)
			local mod = function(cid,mid)
				Abils.cantBeBlocked(cid)
				Abils.destroyModAtEOT(cid,mid)
			end
			local size = getZoneSize(owner,ZONE_BATTLE)
			for i=0,(size-1) do
				local creature = getCardAt(owner,ZONE_BATTLE,i)
				if(cardHasCivilization(creature,CIV_WATER)) then
					createModifier(creature,mod)
				end
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Battleship Mutant"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local tap = function(id)
			local owner = getCardOwner(id)
			local mod = function(cid,mid)
				Abils.bonusPower(cid,4000)
				Abils.Breaker(cid,2)
				Abils.destroyAfterBattle(cid)
				Abils.destroyModAtEOT(cid,mid)
			end
			local size = getZoneSize(owner,ZONE_BATTLE)
			for i=0,(size-1) do
				local creature = getCardAt(owner,ZONE_BATTLE,i)
				if(cardHasCivilization(creature,CIV_DARKNESS)) then
					createModifier(creature,mod)
				end
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Armored Transport Galiacruse"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local owner = getCardOwner(id)
			local mod = function(cid,mid)
				Abils.canAttackUntappedCreatures(cid)
				Abils.destroyModAtEOT(cid,mid)
			end
			local size = getZoneSize(owner,ZONE_BATTLE)
			for i=0,(size-1) do
				local creature = getCardAt(owner,ZONE_BATTLE,i)
				if(cardHasCivilization(creature,CIV_FIRE)) then
					createModifier(creature,mod)
				end
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Spinning Totem"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local tap = function(id)
			local owner = getCardOwner(id)
			local mod = function(cid,mid)
				if(getMessageType()=="post creaturebattle" and getMessageInt("blocked")==1) then
					local attacker = getMessageInt("attacker")
					if(getCardOwner(attacker)==owner and cardHasCivilization(attacker,CIV_NATURE)) then
						local ch = createChoice("Choose an opponent's shield",0,id,owner,Checks.InOppShields)
						if(ch>=0) then
							creatureBreakShield(attacker,ch)
						end
					end
				end
				Abils.destroyModAtEOT(cid,mid)
			end
			createModifier(id,mod)
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Bex, the Oracle"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="get creatureisblocker") then
			if(getMessageInt("creature")==id) then
				local c = getZoneSize(getCardOwner(id), ZONE_SHIELD)
				if(c<=0) then
					setMessageInt("isblocker",1)
				end
			end
		end
	end
}

Cards["Geoshine, Spectral Knight"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local check = function(cid,sid)
			if(Checks.UntappedInBattle(cid,sid)==1 and (cardHasCivilization(sid,CIV_FIRE) or cardHasCivilization(sid,CIV_DARKNESS))) then
				return 1
			else
				return 0
			end
		end
		local func = function(id)
			local ch = createChoice("Choose a fire or darkness creature",1,id,getCardOwner(id),check)
			if(ch>=0) then
				tapCard(ch)
			end
        end
		Abils.onAttack(id,func)
	end
}

Cards["Justice Jamming"] = {
	price_tier = 3,
	shieldtrigger = 0,

	OnCast = function(id)
		local owner = getCardOwner(id)
		local choice = createChoiceNoCheck("Tap darkness creatures? (Choose No for fire)",2,id,owner,Checks.False)
		local civilization = CIV_DARKNESS
		if(choice==RETURN_BUTTON2) then
			civilization = CIV_FIRE
		end
		local tapCivilization = function(cid,sid)
			if(cardHasCivilization(sid,civilization)) then
				tapCard(sid)
			end
		end
		Functions.executeForCreaturesInBattle(id,owner,tapCivilization)
		Functions.executeForCreaturesInBattle(id,getOpponent(owner),tapCivilization)
		Functions.EndSpell(id)
	end
}

Cards["Kizar Basiku, the Outrageous"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id,"Initiate")
		Abils.Stealth(id,CIV_FIRE)
	end
}

Cards["Lightning Charger"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id)
        local ch = createChoice("Choose an opponent's creature",1,id,getCardOwner(id),Checks.UntappedInOppBattle)
        if(ch>=0) then
			tapCard(ch)
        end
        Functions.EndSpell(id)
	end,

    HandleMessage = function(id)
        Abils.Charger(id)
    end
}

Cards["Miracle Portal"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local mod = function(cid,mid)
			Abils.cantBeBlocked(cid)
			Abils.cantAttackCreatures(cid)
			Abils.canAttackPlayersAlways(cid)
			Abils.destroyModAtEOT(cid,mid)
		end
		local ch = createChoice("Choose one of your creatures",0,id,getCardOwner(id),Checks.InYourBattle)
		if(ch>=0) then
			createModifier(ch,mod)
		end
		Functions.EndSpell(id)
	end
}

Cards["Pulsar Tree"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="mod breakshield" and getMessageInt("msgContinue")~=0 and getCardZone(id)==ZONE_BATTLE and getMessageInt("player")==getCardOwner(id)) then
			local ch = createChoiceNoCheck("Destroy Pulsar Tree instead?",2,id,getCardOwner(id),Checks.False)
			if(ch==RETURN_BUTTON1) then
				setMessageInt("msgContinue",0)
				destroyCreature(id)
			end
		end
	end
}

Cards["Rodi Gale, Night Guardian"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Stealth(id,CIV_DARKNESS)
	end
}

Cards["Rom, Vizier of Tendrils"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
            local ch = createChoice("Choose an opponent's creature",1,id,getCardOwner(id),Checks.UntappedInOppBattle)
            if(ch>=0) then
                tapCard(ch)
            end
        end
        Abils.onSummon(id,summon)
	end
}

Cards["Rondobil, the Explorer"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local owner = getCardOwner(id)
			local preferred = Functions.HighestCostShieldTriggerChoice(id,owner,ZONE_BATTLE,Checks.InYourBattle)
			if(preferred==RETURN_NOTHING) then
				preferred = Functions.LowestBattleValueChoice(id,owner,ZONE_BATTLE,Checks.InYourBattle)
			end
			local ch = createChoice("Choose one of your creatures",0,id,owner,Checks.InYourBattle,preferred)
			if(ch>=0) then
				moveCard(ch,ZONE_SHIELD)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Aqua Agent"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Stealth(id,CIV_WATER)
		Abils.returnAfterDestroyed(id)
	end
}

Cards["Aqua Fencer"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local ch = createChoice("Choose a card in your opponent's manazone",0,id,getCardOwner(id),Checks.InOppMana)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Biancus"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local mod = function(cid,mid)
				Abils.cantBeBlocked(cid)
				Abils.destroyModAtEOT(cid,mid)
			end
			local ch = createChoice("Choose creature",0,id,getCardOwner(id),Checks.InYourBattle)
			if(ch>=0) then
				createModifier(ch,mod)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Cetibols"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local func = function(id)
			drawCards(getCardOwner(id),count)
		end
		Abils.onDestroy(id,func)
	end
}

Cards["Curious Eye"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local attack = function(id)
			local owner = getCardOwner(id)
			local ch = createChoice("Choose an opponent's shield to look at",1,id,owner,Checks.InOppShields)
			if(ch>=0) then
				unflipCard(ch)
				setCardVisibility(ch,owner,1)
				createChoiceNoCheck("Look at the shield",1,id,owner,Checks.False)
				flipCard(ch)
				setCardVisibility(ch,owner,0)
			end
		end
		Abils.onAttack(id,attack)
	end
}

Cards["Garatyano"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local tap = function(id)
			local owner = getCardOwner(id)
			local size = getZoneSize(owner,ZONE_DECK)
			local cards = {}
			for i=1,math.min(3,size) do
				local card = getCardAt(owner,ZONE_DECK,size-i)
				cards[card] = true
				unflipCard(card)
				setCardVisibility(card,owner,1)
			end
			local selected = {}
			local valid = function(cid,sid)
				if(cards[sid]==true and selected[sid]~=true) then
					return 1
				end
				return 0
			end
			for i=1,math.min(3,size) do
				local ch = createChoice("Choose the next card, from bottom to top",0,id,owner,valid)
				if(ch>=0) then
					selected[ch] = true
					moveCard(ch,ZONE_DECK)
				end
			end
			for card in pairs(cards) do
				flipCard(card)
				setCardVisibility(card,owner,0)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Riptide Charger"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id)
		local ch = createChoice("Choose a creature",0,id,getCardOwner(id),Checks.InBattle)
	    if(ch>=0) then
            moveCard(ch,ZONE_HAND)
        end
        Functions.EndSpell(id)
	end,

	HandleMessage = function(id)
		Abils.Charger(id)
		Abils.AiCanCastIfValidTarget(id,getOpponent(getCardOwner(id)),ZONE_BATTLE,Checks.InBattle)
	end
}

Cards["Splash Zebrafish"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local func = function(id)
            local ch = createChoice("Choose a card in your mana zone",0,id,getCardOwner(id),Checks.InYourMana)
            if(ch>=0) then
                moveCard(ZONE_HAND)
            end
        end
		Abils.onSummon(id,func)
		Abils.cantBeBlocked(id)
	end
}

Cards["Titanium Cluster"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantBeAttacked(id)
		Abils.cantAttack(id)
	end
}

Cards["Trenchdive Shark"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local owner = getCardOwner(id)
			local selected = {}
			local validHand = function(cid,sid)
				if(Checks.InYourHand(cid,sid)==1 and selected[sid]~=true) then
					return 1
				end
				return 0
			end
			local count = 0
			for i=1,2 do
				local preferred = Functions.HighestCostShieldTriggerChoice(id,owner,ZONE_HAND,validHand)
				if(preferred==RETURN_NOTHING) then
					preferred = Functions.LowestHandValueChoice(id,owner,ZONE_HAND,validHand)
				end
				local ch = createChoice("Choose a card from your hand to add to your shields",1,id,owner,validHand,preferred)
				if(ch<0) then
					break
				end
				selected[ch] = true
				moveCard(ch,ZONE_SHIELD)
				count = count+1
			end
			local shieldSelected = {}
			local validShield = function(cid,sid)
				if(Checks.InYourShields(cid,sid)==1 and shieldSelected[sid]~=true) then
					return 1
				end
				return 0
			end
			for i=1,count do
				local ch = createChoice("Choose a shield to put into your hand",0,id,owner,validShield)
				if(ch>=0) then
					shieldSelected[ch] = true
					moveCard(ch,ZONE_HAND)
				end
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Dream Pirate, Shadow of Theft"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="mod creaturedestroy" and getMessageInt("creature")==id and getCardZone(id)==ZONE_BATTLE) then
			local owner = getCardOwner(id)
			local ch = createChoiceNoCheck("Return Dream Pirate to your hand instead?",2,id,owner,Checks.False)
			if(ch==RETURN_BUTTON1) then
				local mod = function(cid,mid)
					if(getMessageType()=="post cardmove" and getMessageInt("card")==cid and getMessageInt("to")==ZONE_HAND) then
						local discard = createChoice("Choose a card from your hand to discard",0,cid,owner,Checks.InYourHand)
						if(discard>=0) then
							discardCard(discard)
						end
						destroyModifier(cid,mid)
					end
				end
				createModifier(id,mod)
				setMessageInt("zoneto",ZONE_HAND)
			end
		end
	end
}

Cards["Gezary, Undercover Doll"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Stealth(id,CIV_NATURE)
	end
}

Cards["Gigabuster"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttack(id)

		local summon = function(id)
			local ch = createChoice("Choose a shield", 0, id, getCardOwner(id), Checks.InYourShields)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Hopeless Vortex"] = {
	price_tier = 3,
	shieldtrigger = 0,

	HandleMessage = function(id)
		Abils.AiCanCastIfValidTarget(id,getOpponent(getCardOwner(id)),ZONE_BATTLE,Checks.InOppBattle)
	end,

	OnCast = function(id)
		local ch = createChoice("Choose an opponent's creature",0,id,getCardOwner(id),Checks.InOppBattle)
	    if(ch>=0) then
            destroyCreature(ch)
        end
	end
}

Cards["Phantasmal Horror Gigazabal"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id, "Chimera")
		Abils.Stealth(id, CIV_LIGHT)
	end
}

Cards["Propeller Mutant"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local func = function(id)
			discardCardAtRandom(getOpponent(getCardOwner(id)))
		end
		Abils.onDestroy(id,func)
	end
}

Cards["Scalpel Spider"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="post creatureattack" and getMessageInt("defendertype")==DEFENDER_CREATURE and getMessageInt("defender")==id and getCardZone(id)==ZONE_BATTLE) then
			local mod = function(cid,mid)
				Abils.Slayer(cid)
				Abils.destroyModAtEOT(cid,mid)
			end
			createModifier(id,mod)
		end
	end
}

Cards["Three-Faced Ashura Fang"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local ch = createChoice("Choose a shield", 0, id, getCardOwner(id), Checks.InYourShields)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Vacuum Gel"] = { --test
	price_tier = 1,
	shieldtrigger = 1,

	HandleMessage = function(id)
		local valid = function(cid,sid)
			if(Checks.UntappedInOppBattle(cid,sid)==1 and
				(cardHasCivilization(sid,CIV_NATURE) or cardHasCivilization(sid,CIV_LIGHT))) then return 1 end
			return 0
		end
		Abils.AiRemovalTarget(id,valid)
	end,

	OnCast = function(id)
		local check = function(cid,sid)
			if(Checks.UntappedInOppBattle(cid,sid)==1) then
				if(cardHasCivilization(sid,CIV_NATURE) or cardHasCivilization(sid,CIV_LIGHT)) then
					return 1
				else
					return 0
				end
			else
				return 0
			end
		end

		local ch = createChoice("Choose an opponent's creature",0,id,getCardOwner(id),check)
	    if(ch>=0) then
            destroyCreature(ch)
        end
        Functions.EndSpell(id)
	end
}

Cards["Venom Charger"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id)
		local mod = function(cid,mid)
			Abils.Slayer(cid)
			Abils.destroyModAtEOT(cid,mid)
		end
		local ch = createChoice("Choose one of your creatures",0,id,getCardOwner(id),Checks.InYourBattle)
		if(ch>=0) then
			createModifier(ch,mod)
		end
		Functions.EndSpell(id)
	end,

	HandleMessage = function(id)
		Abils.Charger(id)
	end
}

Cards["Apocalypse Vise"] = {
	price_tier = 3,
	shieldtrigger = 0,

	HandleMessage = function(id)
		Abils.AiCanCastIfValidTarget(id,getOpponent(getCardOwner(id)),ZONE_BATTLE,function(cid,sid)
			if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=8000) then return 1 end
			return 0
		end)
	end,

	OnCast = function(id)
		local owner = getCardOwner(id)
		local selected = {}
		local remaining = 8000
		local valid = function(cid,sid)
			if(Checks.InOppBattle(cid,sid)==1 and selected[sid]~=true and getCreaturePower(sid)<=remaining) then
				return 1
			end
			return 0
		end
		while(remaining>0) do
			local ch = createChoice("Choose an opponent's creature to destroy",1,id,owner,valid)
			if(ch<0) then
				break
			end
			selected[ch] = true
			remaining = remaining-getCreaturePower(ch)
			destroyCreature(ch)
		end
		Functions.EndSpell(id)
	end
}

Cards["Astronaut Skyterror"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,
	
	HandleMessage = function(id) --test
		if(getMessageType()=="get creaturepower") then
			if(getMessageInt("creature")==id and getAttacker()==id) then
				local c = getZoneSize(getCardOwner(id),ZONE_BATTLE)
				if(c<=1) then
					setMessageInt("power",getMessageInt("power")+4000)
				end
			end
		end

		if(getMessageType()=="get creaturebreaker") then
			if(getMessageInt("creature")==id) then
				local c = getZoneSize(getCardOwner(id),ZONE_BATTLE)
				if(getMessageInt("breaker") < 2 and c<=1) then
					setMessageInt("breaker",2)
				end
			end
		end
	end
}

Cards["Cratersaur"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="get creaturepower") then
			if(getMessageInt("creature")==id and getAttacker()==id) then
				local c = getZoneSize(getCardOwner(id),ZONE_SHIELD)
				if(c<=0) then
					setMessageInt("power",getMessageInt("power")+3000)
				end
			end
		end

		if(getMessageType()=="get creaturecanattackcreature") then
			if(getMessageInt("attacker")==id) then
				local c = getZoneSize(getCardOwner(id),ZONE_SHIELD)
				if(c<=0) then
					setMessageInt("canattack",CANATTACK_UNTAPPED)
				end
			end
		end
	end
}

Cards["Energy Charger"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id)
		local mod = function(cid,mid)
            Abils.PowerAttacker(cid,2000)
		    Abils.destroyModAtEOT(cid,mid)
        end
		local c = createChoice("Choose creature",0,id,getCardOwner(id),Checks.InYourBattle)
		if(c>=0) then
            createModifier(c,mod)
        end
	end,

	HandleMessage = function(id)
		Abils.Charger(id)
	end
}

Cards["Gazarias Dragon"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="get creaturepower") then
			if(getMessageInt("creature")==id) then
				local c = getZoneSize(getCardOwner(id),ZONE_SHIELD)
				if(c<=0) then
					setMessageInt("power",getMessageInt("power")+4000)
				end
			end
		end

		if(getMessageType()=="get creaturebreaker") then
			if(getMessageInt("creature")==id) then
				local c = getZoneSize(getCardOwner(id),ZONE_SHIELD)
				if(getMessageInt("breaker") < 2 and c<=0) then
					setMessageInt("breaker",2)
				end
			end
		end
	end
}

Cards["Kipo's Contraption"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
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
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Kooc Pollon"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantBeAttacked(id)
	end
}

Cards["Otherworldly Warrior Naglu"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.cantBeAttacked(id)
		Abils.PowerAttacker(id,3000)
	end
}

Cards["Valkrowzer, Ultra Rock Beast"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id, "Rock Beast")
		Abils.Stealth(id, CIV_WATER)
	end
}

Cards["Wild Racer Chief Garan"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.PowerAttacker(id,1000)
		Abils.Stealth(id,CIV_LIGHT)
	end
}

Cards["Brood Shell"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local ch = createChoice("Choose a creature in your mana zone",0,id,getCardOwner(id),Checks.CreatureInYourMana)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Cursed Totem"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		if(getMessageType()=="get canuseshieldtrigger" and getCardZone(id)==ZONE_BATTLE) then
			local trigger = getMessageInt("card")
			if(getCardOwner(trigger)~=getCardOwner(id)) then
				setMessageInt("canuse",0)
			end
		end
	end
}

Cards["Freezing Icehammer"] = {
	price_tier = 1,
	shieldtrigger = 0,

	HandleMessage = function(id)
		Abils.AiCanCastIfValidTarget(id,getOpponent(getCardOwner(id)),ZONE_BATTLE,function(cid,sid)
			if(Checks.InOppBattle(cid,sid)==1 and
				(cardHasCivilization(sid,CIV_WATER) or cardHasCivilization(sid,CIV_DARKNESS))) then return 1 end
			return 0
		end)
	end,

	OnCast = function(id)
		local valid = function(cid,sid)
			if(Checks.InOppBattle(cid,sid)==1 and (cardHasCivilization(sid,CIV_WATER) or cardHasCivilization(sid,CIV_DARKNESS))) then
				return 1
			end
			return 0
		end
		local ch = createChoice("Choose an opponent's water or darkness creature",0,id,getCardOwner(id),valid)
		if(ch>=0) then
			moveCard(ch,ZONE_MANA)
		end
		Functions.EndSpell(id)
	end
}

Cards["Fruit of Eternity"] = {
	price_tier = 2,
	shieldtrigger = 1,

	OnCast = function(id) --test
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
		Functions.EndSpell(id)
	end
}

Cards["Launch Locust"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="get creaturepower") then
			if(getMessageInt("creature")==id and getAttacker()==id) then
				local c = getZoneSize(getCardOwner(id),ZONE_BATTLE)
				setMessageInt("power",getMessageInt("power")+1000*(c-1))
			end
		end
	end
}

Cards["Mulch Charger"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id)
		local ch = createChoice("Choose one of your creatures",0,id,getCardOwner(id),Checks.InYourBattle)
		if(ch>=0) then
			moveCard(ch,ZONE_MANA)
		end
		Functions.EndSpell(id)
	end,

	HandleMessage = function(id)
		Abils.Charger(id)
	end
}

Cards["Popple, Flowerpetal Dancer"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			Functions.moveTopCardsFromDeck(getCardOwner(id), ZONE_MANA, 1)
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Stinger Horn, the Delver"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.PowerAttacker(id,1000)
		Abils.Stealth(id,CIV_WATER)
	end
}

Cards["Tangle Fist, the Weaver"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local tap = function(id)
			local ch1 = createChoice("Choose a card in your hand", 1, id, getCardOwner(id), Checks.InYourHand)
			if(ch1>=0) then
				moveCard(ch1,ZONE_MANA)

				local ch2 = createChoice("Choose a card in your hand", 1, id, getCardOwner(id), Checks.InYourHand)
				if(ch2>=0) then
					moveCard(ch2,ZONE_MANA)

					local ch3 = createChoice("Choose a card in your hand", 1, id, getCardOwner(id), Checks.InYourHand)
					if(ch3>=0) then
						moveCard(ch3,ZONE_MANA)
					end
				end
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["World Tree, Root of Life"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id,"Tree Folk")
		Abils.PowerAttacker(id,2000)
		Abils.Stealth(id,CIV_DARKNESS)
	end
}
