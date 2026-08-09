package.path = package.path .. ';./?.lua;'
require("Lua/Survivors")

local requireSpellCast = function(id)
	if(getMessageType()=="post startturn") then
		clearDuelState("invincible_soul.spell_cast_this_turn",getMessageInt("player"))
	elseif(getMessageType()=="post cardmove") then
		local card = getMessageInt("card")
		if(getMessageInt("to")==ZONE_BATTLE and getCardType(card)==TYPE_SPELL) then
			setDuelStateInt("invincible_soul.spell_cast_this_turn",getCardOwner(card),1)
		end
	elseif(getMessageType()=="get cardcancast" and getMessageInt("card")==id) then
		if(getDuelStateInt("invincible_soul.spell_cast_this_turn",getCardOwner(id),0)~=1) then
			setMessageInt("cancast",0)
		end
	end
end

local gainBlockerWhenOpponentPlays = function(id)
	if(getMessageType()=="post cardmove" and getCardZone(id)==ZONE_BATTLE) then
		local card = getMessageInt("card")
		if(getMessageInt("to")==ZONE_BATTLE and getCardOwner(card)~=getCardOwner(id)) then
			local mod = function(cid,mid)
				Abils.Blocker(cid)
				Abils.destroyModAtEOT(cid,mid)
			end
			createModifier(id,mod)
		end
	end
end

Cards["Craze Valkyrie, the Drastic"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.Evolution(id,"Initiate")
		local func = function(id)
			local ch = createChoice("Choose an opponent's creature",1,id,getCardOwner(id),Checks.UntappedInOppBattle)
            if(ch>=0) then
                tapCard(ch)
				local ch2 = createChoice("Choose an opponent's creature",1,id,getCardOwner(id),Checks.UntappedInOppBattle)
				if(ch2>=0) then
					tapCard(ch2)
				end
            end
		end
		Abils.onSummon(id, func)
	end
}

Cards["Ultra Mantis, Scourge of Fate"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id,"Giant Insect")
		Abils.cantBeBlockedPower(id,8000)
	end
}

Cards["Laveil, Seeker of Catastrophe"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 2,

	HandleMessage = function(id)
		Abils.untapAtEOT(id)
	end
}

Cards["Crystal Jouster"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id,"Liquid People")
		Abils.returnAfterDestroyed(id)
	end
}

Cards["Q-tronic Hypermind"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id, "Survivor")
		local func = function(id)
			local owner = getCardOwner(id)
			local c = Functions.countInZone(owner,ZONE_BATTLE,Checks.IsSurvivor)
			drawCards(owner, c)
		end
		Abils.onSummon(id, func)
	end
}

Cards["Daidalos, General of Fury"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		local func = function(id)
			local ch = createChoice("Choose a creature in your battlezone",0,id,getCardOwner(id),Checks.InYourBattle)
			if(ch>=0) then
				destroyCreature(ch)
			end
		end
		Abils.onAttack(id,func)

		if(getMessageType()=="get creaturecanattackcreature") then
			if(getMessageInt("attacker")==id and Functions.countCreaturesInBattle(getCardOwner(id))<=1) then
				setMessageInt("canattack",CANATTACK_NO)
			end
		end
	end
}

Cards["Phantasmal Horror Gigazald"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Evolution(id, "Chimera")
		local tap = function(id)
			discardCardAtRandom(getOpponent(getCardOwner(id)))
		end
		Abils.TapAbilityForCiv(id, tap, CIV_DARKNESS)
	end
}

Cards["Bolmeteus Steel Dragon"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		if(getMessageType()=="pre creaturebreakshield") then
			if(getMessageInt("creature")==id) then
				setMessageInt("msgContinue", 0)
				moveCard(getMessageInt("shield"), ZONE_GRAVEYARD)
			end
		end
	end
}

Cards["Lava Walker Executo"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Evolution(id,"Dragonoid")
		local tap = function(id)
			local mod = function(cid,mid)
				Abils.PowerAttacker(cid,3000)
				Abils.destroyModAtEOT(cid,mid)
			end
			local c = createChoice("Choose creature",0,id,getCardOwner(id),Checks.InYourBattle)
			if(c>=0) then
				createModifier(c,mod)
			end
		end
		Abils.TapAbilityForCiv(id,tap,CIV_FIRE)
	end
}

Cards["Cliffcrush Giant"] = {
	price_tier = 5,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.PowerAttacker(id,3000)
		if(getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==0 and getMessageInt("attacker")==id) then
			if(getMessageType()=="get creaturecanattackcreature" or getMessageType()=="get creaturecanattackplayers") then
				local owner = getCardOwner(id)
				local size = getZoneSize(owner,ZONE_BATTLE)
				for i=0,(size-1) do
					local creature = getCardAt(owner,ZONE_BATTLE,i)
					if(creature~=id and isCardTapped(creature)==0) then
						setMessageInt("canattack",CANATTACK_NO)
						break
					end
				end
			end
		end
	end
}

Cards["Invincible Aura"] = {
	price_tier = 4,
	shieldtrigger = 0,

	OnCast = function(id)
		Functions.moveTopCardsFromDeck(getCardOwner(id), ZONE_SHIELD, 3)
		Functions.EndSpell(id)
	end
}

Cards["Lu Gila, Silver Rift Guardian"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttackPlayers(id)
		if(getMessageType()=="post cardmove") then
			local evo = getMessageInt("card")
			if(getCreatureIsEvolution(evo)==1 and getMessageInt("to")==ZONE_BATTLE) then
				tapCard(evo)
			end
		end
	end
}

Cards["Aeropica"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local ch = createChoice("Choose a creature",0,id,getCardOwner(id),Checks.InBattle)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Invincible Technology"] = {
	price_tier = 4,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local owner = getCardOwner(id)
        openDeck(owner)
		while(true) do
			local ch = createChoice("Choose a card in your deck",1,id,owner,Checks.InYourDeck)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
			if(ch<0) then
				break
			end
		end
		shuffleDeck(owner)
        closeDeck(owner)
        Functions.EndSpell(id)
	end
}

Cards["Invincible Abyss"] = {
	price_tier = 4,
	shieldtrigger = 0,

	OnCast = function(id)
		local func = function(cid,sid)
			destroyCreature(sid)
		end
		Functions.executeForCreaturesInBattle(id, getOpponent(getCardOwner(id)), func)
		Functions.EndSpell(id)
	end
}

Cards["Tank Mutant"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local ch = createChoice("Choose a creature in your battlezone",0,id,getOpponet(getCardOwner(id)),Checks.InOppBattle)
			if(ch>=0) then
				destroyCreature(ch)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Invincible Cataclysm"] = {
	price_tier = 4,
	shieldtrigger = 0,

	OnCast = function(id)
		local ch = createChoice("Choose an opponent's shield", 1, id, getCardOwner(id), Checks.InOppShields)
		if(ch>=0) then
			moveCard(ch, ZONE_GRAVEYARD)

			local ch2 = createChoice("Choose an opponent's shield", 1, id, getCardOwner(id), Checks.InOppShields)
			if(ch2>=0) then
				moveCard(ch, ZONE_GRAVEYARD)

				local ch3 = createChoice("Choose an opponent's shield", 1, id, getCardOwner(id), Checks.InOppShields)
				if(ch3>=0) then
					moveCard(ch, ZONE_GRAVEYARD)
				end
			end
		end
		Functions.EndSpell(id)
	end
}

Cards["Valiant Warrior Exorious"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.canAttackUntappedCreatures(id)
		Abils.PowerAttacker(id,3000)
	end
}

Cards["Invincible Unity"] = {
	price_tier = 4,
	shieldtrigger = 0,

	OnCast = function(id)
		local mod = function(cid,mid)
            Abils.PowerAttacker(cid,8000)
			Abils.Breaker(cid,3)
		    Abils.destroyModAtEOT(cid,mid)
        end
        local func = function(cid,sid)
            createModifier(sid,mod)
        end
		Functions.executeForCreaturesInBattle(id,getCardOwner(id),func)
        Functions.EndSpell(id)
	end
}

Cards["Splinterclaw Wasp"] = {
	price_tier = 4,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.PowerAttacker(id,3000)
		if(getMessageType()=="post creaturebattle" and getMessageInt("blocked")==1 and getMessageInt("attacker")==id) then
			local ch = createChoice("Choose an opponent's shield",0,id,getCardOwner(id),Checks.InOppShields)
			if(ch>=0) then
				creatureBreakShield(id,ch)
			end
		end
	end
}

Cards["Adomis, the Oracle"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local owner = getCardOwner(id)
			local ch = createChoice("Choose one of your shields to look at",0,id,owner,Checks.InYourShields)
			if(ch>=0) then
				unflipCard(ch)
				setCardVisibility(ch,owner,1)
				createChoiceNoCheck("Look at your shield",1,id,owner,Checks.False)
				flipCard(ch)
				setCardVisibility(ch,owner,0)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Arc Bine, the Astounding"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Evolution(id,"Guardian")

		local tap = function(id)
			local ch = createChoice("Choose an opponent's creature",1,id,getCardOwner(id),Checks.UntappedInOppBattle)
			if(ch>=0) then
				tapCard(ch)
			end
		end
		Abils.TapAbilityForCiv(id,tap,CIV_LIGHT)
	end
}

Cards["Ballas, Vizier of Electrons"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Bonds of Justice"] = {
	price_tier = 3,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local func = function(cid,sid)
			if(getCreatureIsBlocker(sid)==0) then
				tapCard(sid)
			end
		end
		Functions.executeForCreaturesInBattle(id,getCardOwner(id),func)
		Functions.executeForCreaturesInBattle(id,getOpponent(getCardOwner(id)),func)
		Functions.EndSpell(id)
	end
}

Cards["Chekicul, Vizier of Endurance"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.cantAttack(id)
		if(getMessageType()=="mod creaturebattle" and getMessageInt("blocked")==1 and getMessageInt("defender")==id) then
			setMessageInt("msgContinue",0)
		end
	end
}

Cards["Chen Treg, Vizier of Blades"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local ch = createChoice("Choose an opponent's creature",0,id,getCardOwner(id),Checks.UntappedInOppBattle)
			if(ch>=0) then
				tapCard(ch)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Cosmogold, Spectral Knight"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local ch = createChoice("Choose a spell in your mana zone",0,id,getCardOwner(id),Checks.SpellInYourMana)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Dava Torey, Seeker of Clouds"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="mod carddiscard" and getMessageInt("card")==id and getCardZone(id)==ZONE_HAND) then
			local owner = getCardOwner(id)
			if(getTurn()~=owner) then
				setMessageInt("zoneto",ZONE_BATTLE)
			end
		end
	end
}

Cards["Forbos, Sanctum Guardian Q"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local func = function(id)
			local summon = function(id)
				local owner = getCardOwner(id)
				openDeck(owner)
				local ch = createChoice("Choose a spell in your deck",1,id,owner,Checks.SpellInYourDeck)
				closeDeck(owner)
				if(ch>=0) then
					moveCard(ch,ZONE_HAND)
					shuffleDeck(getCardOwner(ch))
				end
			end
			Abils.onSummon(id,summon)
		end
		Abils.Survivor(id,func)
	end
}

Cards["Gariel, Elemental of Sunbeams"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		requireSpellCast(id)
	end
}

Cards["Kanesill, the Explorer"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttackPlayers(id)
	end
}

Cards["Lightning Grass"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Moontear, Spectral Knight"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		requireSpellCast(id)
	end
}

Cards["Protective Force"] = {
	price_tier = 2,
	shieldtrigger = 1,

	OnCast = function(id)
		local mod = function(cid,mid)
			Abils.bonusPower(cid, 4000)
			Abils.destroyModAtEOT(cid,mid)
		end
		
		local ch = createChoice("Choose a blocker", 0, id, getCardOwner(id), Checks.BlockerInYourBattle)
		if(ch>=0) then
			createModifier(ch, mod)
		end
		Functions.EndSpell(id)
	end
}

Cards["Rain of Arrows"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local owner = getCardOwner(id)
		local opponent = getOpponent(owner)
		local hand = {}
		local size = getZoneSize(opponent,ZONE_HAND)
		for i=0,(size-1) do
			local card = getCardAt(opponent,ZONE_HAND,i)
			hand[#hand+1] = card
			unflipCard(card)
			setCardVisibility(card,owner,1)
		end
		createChoiceNoCheck("Look at your opponent's hand",1,id,owner,Checks.False)
		for _,card in ipairs(hand) do
			if(getCardZone(card)==ZONE_HAND and getCardType(card)==TYPE_SPELL and getCardCiv(card)==CIV_DARKNESS) then
				discardCard(card)
			elseif(getCardZone(card)==ZONE_HAND) then
				flipCard(card)
				setCardVisibility(card,owner,0)
			end
		end
		Functions.EndSpell(id)
	end
}

Cards["Razorpine Tree"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="get creaturepower") then
			if(getMessageInt("creature")==id) then
				local c = Functions.countInZone(id,getCardOwner(id),ZONE_SHIELD,Checks.True)
				setMessageInt("power",getMessageInt("power")+c*2000)
			end
		end
	end
}

Cards["Sphere of Wonder"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local owner = getCardOwner(id)
        local c1 = getZoneSize(owner, ZONE_SHIELD)
        local c2 = getZoneSize(getOpponent(owner), ZONE_SHIELD)
        if(c2>c1) then
            Functions.moveTopCardsFromDeck(owner,ZONE_SHIELD,1)
        end
        Functions.EndSpell(id)
	end
}

Cards["Telitol, the Explorer"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.cantAttackPlayers(id)
		local summon = function(id)
			local owner = getCardOwner(id)
			local ch = createChoiceNoCheck("Look at your shields?",2,id,owner,Checks.False)
			if(ch==RETURN_BUTTON1) then
				local shields = {}
				local size = getZoneSize(owner,ZONE_SHIELD)
				for i=0,(size-1) do
					local card = getCardAt(owner,ZONE_SHIELD,i)
					shields[#shields+1] = card
					unflipCard(card)
					setCardVisibility(card,owner,1)
				end
				createChoiceNoCheck("Look at your shields",1,id,owner,Checks.False)
				for _,card in ipairs(shields) do
					flipCard(card)
					setCardVisibility(card,owner,0)
				end
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Vess, the Oracle"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttackPlayers(id)
	end
}

Cards["Yuluk, the Oracle"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		requireSpellCast(id)
	end
}

Cards["Aqua Rider"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		gainBlockerWhenOpponentPlays(id)
	end
}

Cards["Energy Stream"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id)
		drawCards(getCardOwner(id), 2)
		Functions.EndSpell(id)
	end
}

Cards["Fort Megacluster"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Evolution(id, "Cyber Cluster")
		local tap = function(id)
			drawCards(getCardOwner(id),1)
		end
		Abils.TapAbilityForCiv(id, tap, CIV_WATER)
	end
}

Cards["Hazard Crawler"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttack(id)
	end
}

Cards["King Triumphant"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		gainBlockerWhenOpponentPlays(id)
	end
}

Cards["Kyuroro"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="mod creaturebreakshield" and getCardZone(id)==ZONE_BATTLE) then
			local owner = getCardOwner(id)
			if(getMessageInt("defender")==owner) then
				local ch = createChoice("Choose which shield is broken",0,id,owner,Checks.InYourShields)
				if(ch>=0) then
					setMessageInt("shield",ch)
				end
			end
		end
	end
}

Cards["Madrillon Fish"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttack(id)
	end
}

Cards["Midnight Crawler"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		local summon = function(id)
			local ch = createChoice("Choose a card in your opponent's mana zone",0,id,getCardOwner(id),Checks.InOppMana)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Mystic Dreamscape"] = {
	price_tier = 2,
	shieldtrigger = 1,

	OnCast = function(id)
		local ch = createChoice("Choose a card in your mana zone",1,id,getCardOwner(id),Checks.InYourMana)
        if(ch>=0) then
			moveCard(ZONE_HAND)
			
			local ch2 = createChoice("Choose a card in your mana zone",1,id,getCardOwner(id),Checks.InYourMana)
			if(ch2>=0) then
				moveCard(ZONE_HAND)

				local ch3 = createChoice("Choose a card in your mana zone",1,id,getCardOwner(id),Checks.InYourMana)
				if(ch3>=0) then
					moveCard(ZONE_HAND)
				end
			end
        end
		Functions.EndSpell(id)
	end
}

Cards["Neon Cluster"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			drawCards(getCardOwner(id), 2)
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Overload Cluster"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		gainBlockerWhenOpponentPlays(id)
	end
}

Cards["Promephius Q"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Raptor Fish"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local summon = function(id)
			local func = function(cid,sid)
				moveCard(sid,ZONE_DECK)
			end
			local owner = getCardOwner(id)
			local c = getZoneSize(owner, ZONE_HAND)
			Functions.executeForCardsInZone(owner,ZONE_HAND,func)
			shuffleDeck(owner)
			drawCards(owner, c)
		end
		Abils.onSummon(id,summon)
	end
}

Cards["Ripple Lotus Q"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local func = function(id)
			local summon = function(id)
				local ch = createChoice("Choose an opponent's creature",1,id,getCardOwner(id),Checks.UntappedInOppBattle)
				if(ch>=0) then
					tapCard(ch)
				end
			end
			Abils.onSummon(id,summon)
		end
		Abils.Survivor(id,func)
	end
}

Cards["Shock Hurricane"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id) --test
		local count = 0
		while (true) do
			local ch = createChoice("Choose a creature in your battlezone", 1, id, getCardOwner(id), Checks.InYourBattle)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
				count = count+1
			end
			if(ch<0) then
				break
			end
		end

		for i=1,count do
			local ch = createChoice("Choose a creature in your opponent's battlezone", 1, id, getCardOwner(id), Checks.InOppBattle)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
			if(ch<0) then
				break
			end
		end
	end
}

Cards["Sopian"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
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

Cards["Spiral Gate"] = {
	price_tier = 1,
	shieldtrigger = 1,

	OnCast = function(id)
        local ch = createChoice("Choose a creature",0,id,getCardOwner(id),Checks.InBattle)
	    if(ch>=0) then
            moveCard(ch,ZONE_HAND)
        end
        Functions.EndSpell(id)
	end
}

Cards["Steam Star"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Thrash Crawler"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttack(id)

		local func = function(id)
			local ch = createChoice("Choose a card in your mana zone", 0, id, getCardOwner(id), Checks.InYourMana)
			if(ch>=0) then
				moveCard(ch, ZONE_HAND)
			end
		end
		Abils.onSummon(id,func)
	end
}

Cards["Zepimeteus"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttack(id)
	end
}

Cards["Bazooka Mutant"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.cantAttackPlayers(id)
		if(getMessageType()=="get creaturecanattackcreature") then
			if(getMessageInt("attacker")==id and getCreatureIsBlocker(getMessageInt("defender"))==0) then
				setMessageInt("canattack",CANATTACK_NO)
			end
		end
	end
}

Cards["Cursed Pincher"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttack(id)
		Abils.Slayer(id)
	end
}

Cards["Death Smoke"] = {
	price_tier = 1,
	shieldtrigger = 0,

	OnCast = function(id)
		local ch = createChoice("Choose an opponent's creature",0,id,getCardOwner(id),Checks.UntappedInOppBattle)
	    if(ch>=0) then
            destroyCreature(ch)
        end
        Functions.EndSpell(id)
	end
}

Cards["Frost Specter, Shadow of Age"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Evolution(id,"Ghost")
		if(getCardZone(id)==ZONE_BATTLE) then
			local owner = getCardOwner(id)
			local size = getZoneSize(owner,ZONE_BATTLE)
			for i=0,(size-1) do
				local creature = getCardAt(owner,ZONE_BATTLE,i)
				if(isCreatureOfRace(creature,"Ghost")==1) then
					Abils.Slayer(creature)
				end
			end
		end
	end
}

Cards["Future Slash"] = {
	price_tier = 2,
	shieldtrigger = 0,

	OnCast = function(id)
		local owner = getCardOwner(id)
		local opponent = getOpponent(owner)
		local valid = function(cid,sid)
			if(getCardOwner(sid)==opponent and getCardZone(sid)==ZONE_DECK) then
				return 1
			end
			return 0
		end
		openDeck(opponent)
		for i=1,2 do
			local ch = createChoice("Choose a card in your opponent's deck",1,id,owner,valid)
			if(ch<0) then
				break
			end
			moveCard(ch,ZONE_GRAVEYARD)
		end
		shuffleDeck(opponent)
		closeDeck(opponent)
		Functions.EndSpell(id)
	end
}

Cards["Gigagriff"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Slayer(id)
		Abils.cantAttack(id)
	end
}

Cards["Gnarvash, Merchant of Blood"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="pre endturn") then
			if(getMessageInt("player")==getCardOwner(id) and getCardZone(id)==ZONE_BATTLE) then
				local c = Functions.countInZone(getCardOwner(id), ZONE_BATTLE, Checks.True)
				if(c<=1) then
					destroyCreature(id)
				end
			end
		end
	end
}

Cards["Grave Worm Q"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
	end
}

Cards["Grim Soul, Shadow of Reversal"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local check = function(cid,sid)
				if(Checks.CreatureInYourGraveyard(cid,sid)==1 and getCardCiv(sid)==CIV_DARKNESS) then
					return 1
				else
					return 0
				end
			end

			local ch = createChoice("Choose a creature in your graveyard",0,id,getCardOwner(id),check)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Grinning Axe, the Monstrosity"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Slayer(id)
	end
}

Cards["Wicked Soul Reincarnation"] = {
	price_tier = 3,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local owner = getCardOwner(id)
		local count = 0
		local selected = {}
		local valid = function(cid,sid)
			if(Checks.InYourBattle(cid,sid)==1 and selected[sid]~=true) then
				return 1
			end
			return 0
		end
		while(true) do
			local ch = createChoice("Choose one of your creatures to destroy",1,id,owner,valid)
			if(ch<0) then
				break
			end
			selected[ch] = true
			destroyCreature(ch)
			count = count+1
		end
		if(count>0) then
			drawCards(owner,count*2)
		end
		Functions.EndSpell(id)
	end
}

Cards["Junkatz, Rabid Doll"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Lone Tear, Shadow of Solitude"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="pre endturn") then
			if(getMessageInt("player")==getCardOwner(id) and getCardZone(id)==ZONE_BATTLE) then
				local c = Functions.countInZone(getCardOwner(id), ZONE_BATTLE, Checks.True)
				if(c<=1) then
					destroyCreature(id)
				end
			end
		end
	end
}

Cards["Lupa, Poison-Tipped Doll"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local mod = function(cid,mid)
				Abils.Slayer(cid)
				Abils.destroyModAtEOT(cid,mid)
			end
			local ch = createChoice("Choose one of your creatures",0,id,getCardOwner(id),Checks.InYourBattle)
			if(ch>=0) then
				createModifier(ch,mod)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Proclamation of Death"] = {
	price_tier = 2,
	shieldtrigger = 1,

	OnCast = function(id)
		local ch = createChoice("Choose a creature in your battlezone",0,id,getOpponent(getCardOwner(id)),Checks.InOppBattle)
	    if(ch>=0) then
            destroyCreature(ch)
        end
        Functions.EndSpell(id)
	end
}

Cards["Schuka, Duke of Amnesia"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local summon = function(id)
			local func = function(cid,sid)
				moveCard(sid,ZONE_GRAVEYARD)
			end
			local owner = getCardOwner(id)
			Functions.executeForCardsInZone(owner, ZONE_HAND, func)
			Functions.executeForCardsInZone(getOpponent(owner), ZONE_HAND, func)
		end
		Abils.onDestroy(id,summon)
	end
}

Cards["Skullcutter, Swarm Leader"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="pre endturn") then
			if(getMessageInt("player")==getCardOwner(id) and getCardZone(id)==ZONE_BATTLE) then
				local c = Functions.countInZone(getCardOwner(id), ZONE_BATTLE, Checks.True)
				if(c<=1) then
					destroyCreature(id)
				end
			end
		end
	end
}

Cards["Tentacle Worm"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Vile Mulder, Wing of the Void"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.destroyAfterBattle(id)
		Abils.cantAttackCreatures(id)
	end
}

Cards["Zorvaz, the Bonecrusher"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 1,
	breaker = 1,

	HandleMessage = function(id)
		Abils.destroyAfterBattle(id)
		Abils.cantAttack(id)
	end
}

Cards["Armored Decimator Valkaizer"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Evolution(id,"Human")

		local summon = function(id)
			local valid = function(cid,sid)
				if(getCardOwner(sid)~=getCardOwner(cid) and getCardZone(sid)==ZONE_BATTLE and getCreaturePower(sid)<=4000) then
					return 1
				else
					return 0
				end
			end
			local ch = createChoice("Choose an opponent's creature",1,id,getCardOwner(id),valid)
			if(ch>=0) then
				destroyCreature(ch)
			end
		end

		Abils.onSummon(id,summon)
	end
}

Cards["Armored Scout Gestuchar"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local owner = getCardOwner(id)
		local otherFire = false
		local size = getZoneSize(owner,ZONE_BATTLE)
		for i=0,(size-1) do
			local creature = getCardAt(owner,ZONE_BATTLE,i)
			if(creature~=id and getCardType(creature)==TYPE_CREATURE and getCardCiv(creature)==CIV_FIRE) then
				otherFire = true
				break
			end
		end
		if(not otherFire) then
			Abils.PowerAttacker(id,3000)
			Abils.Breaker(id,2)
		end
	end
}

Cards["Automated Weaponmaster Machai"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.attacksEachTurn(id)
	end
}

Cards["Badlands Lizard"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.PowerAttacker(id,3000)
		if(getMessageType()=="mod creaturebattle" and getMessageInt("blocked")==1 and getMessageInt("attacker")==id) then
			setMessageInt("msgContinue",0)
		end
	end
}

Cards["Bazagazeal Dragon"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id) --test
		Abils.returnAtEOT(id)
		Abils.SpeedAttacker(id)
		Abils.canAttackUntappedCreatures(id)
	end
}

Cards["Choya, the Unheeding"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.PowerAttacker(id,1000)
		if(getMessageType()=="mod creaturebattle" and getMessageInt("blocked")==1 and getMessageInt("attacker")==id) then
			setMessageInt("msgContinue",0)
		end
	end
}

Cards["Cocco Lupia"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getMessageType()=="get cardcost") then
            if(getCardZone(id)==ZONE_BATTLE) then
                local card = getMessageInt("card")
                if(getCardType(card)==TYPE_CREATURE and getCardOwner(id)==getCardOwner(card) and isCreatureOfRace(card, "Dragon")==1) then
                    local cost = getMessageInt("cost")
					if(cost-2 > 2) then
						setMessageInt("cost",cost-2)
                    else
						setMessageInt("cost",2)
					end
                end
            end
        end
	end
}

Cards["Comet Missile"] = {
	price_tier = 2,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local check = function(cid,sid)
			if(getCardType(sid)==TYPE_CREATURE and getCreaturePower(sid)<=6000) then
				return Checks.BlockerInOppBattle(cid,sid)
			else
				return 0
			end
		end
		
		local ch = createChoice("Choose an opponent's blocker",1,id,getCardOwner(id),check)
		if(ch>=0) then
			destroyCreature(ch)
		end
	end
}

Cards["Crisis Boulder"] = {
	price_tier = 3,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local check = function(cid,sid)
			return (Checks.InOppMana(cid,sid) or Checks.InOppBattle(cid,sid))
		end
		
		local ch = createChoice("Choose an card in your battle zone or mana zone",0,id,getOpponent(getCardOwner(id)),check)
		if(ch>=0) then
			if(getCardZone(ch)==ZONE_BATTLE) then
				destroyCreature(ch)
			end
			if(getCardZone(ch)==ZONE_MANA) then
				destroyMana(ch)
			end
		end
	end
}

Cards["Cutthroat Skyterror"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		Abils.cantAttackPlayers(id)
		Abils.SpeedAttacker(id)
		Abils.returnAtEOT(id)
	end
}

Cards["Legionnaire Lizard"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.SpeedAttacker(id)
		local tap = function(id)
			local mod = function(cid,mid)
				Abils.SpeedAttacker(cid)
				Abils.destroyModAtEOT(cid,mid)
			end
			local ch = createChoice("Choose one of your creatures",0,id,getCardOwner(id),Checks.InYourBattle)
			if(ch>=0) then
				createModifier(ch,mod)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Migasa, Adept of Chaos"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local mod = function(cid,mid)
				Abils.Breaker(cid,2)
				Abils.destroyModAtEOT(cid,mid)
			end
			local check = function(cid,sid)
				if(Checks.InYourBattle(cid,sid)==1 and getCardCiv(sid)==CIV_FIRE) then
					return 1
				else
					return 0
				end
			end
			local c = createChoice("Choose a fire creature",0,id,getCardOwner(id),check)
			if(c>=0) then
				createModifier(c,mod)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Phantom Dragon's Flame"] = {
	price_tier = 1,
	shieldtrigger = 1,

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
	end
}

Cards["Picora's Wrench"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Pyrofighter Magnus"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.returnAtEOT(id)
		Abils.SpeedAttacker(id)
	end
}

Cards["Q-tronic Gargantua"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Evolution(id,"Survivor")
		local owner = getCardOwner(id)
		local count = Functions.countInZone(id,owner,ZONE_BATTLE,Checks.IsSurvivor)
		Abils.Breaker(id,count)
	end
}

Cards["Rikabu's Screwdriver"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local ch = createChoice("Choose opponent's blocker",0,id,getCardOwner(id),Checks.BlockerInOppBattle)
			if(ch>=0) then
				destroyCreature(ch)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Rumblesaur Q"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local func = function(id)
			Abils.SpeedAttacker(id)
		end
		Abils.Survivor(id,func)
	end
}

Cards["Spastic Missile"] = {
	price_tier = 2,
	shieldtrigger = 0,

	HandleMessage = function(id)
		if(getMessageType()=="get cardaicancast" and getMessageInt("card")==id) then
			local opponent = getOpponent(getCardOwner(id))
			local canDestroy = 0
			local size = getZoneSize(opponent,ZONE_BATTLE)
			for i=0,(size-1) do
				local creature = getCardAt(opponent,ZONE_BATTLE,i)
				if(getCreaturePower(creature)<=3000) then
					canDestroy = 1
					break
				end
			end
			if(canDestroy==0) then setMessageInt("cancast",0) end
		end
	end,

	OnCast = function(id)
		local valid = function(cid,sid)
            if(getCardOwner(sid)~=getCardOwner(cid) and getCardZone(sid)==ZONE_BATTLE and getCreaturePower(sid)<=3000) then
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
	end
}

Cards["Torchclencher"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local owner = getCardOwner(id)
		local size = getZoneSize(owner,ZONE_BATTLE)
		for i=0,(size-1) do
			local creature = getCardAt(owner,ZONE_BATTLE,i)
			if(creature~=id and getCardType(creature)==TYPE_CREATURE and getCardCiv(creature)==CIV_FIRE) then
				Abils.PowerAttacker(id,3000)
				break
			end
		end
	end
}

Cards["Bliss Totem, Avatar of Luck"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		local tap = function(id)
			local ch1 = createChoice("Choose a card in your graveyard", 1, id, getCardOwner(id), Checks.InYourGraveyard)
			if(ch1>=0) then
				moveCard(ch1,ZONE_MANA)

				local ch2 = createChoice("Choose a card in your graveyard", 1, id, getCardOwner(id), Checks.InYourGraveyard)
				if(ch2>=0) then
					moveCard(ch2,ZONE_MANA)

					local ch3 = createChoice("Choose a card in your graveyard", 1, id, getCardOwner(id), Checks.InYourGraveyard)
					if(ch3>=0) then
						moveCard(ch3,ZONE_MANA)
					end
				end
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Cantankerous Giant"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
	end
}

Cards["Carrier Shell"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.PowerAttacker(id,2000)
	end
}

Cards["Charmilia, the Enticer"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local owner = getCardOwner(id)
			openDeck(owner)
			local ch = createChoice("Choose a creature in your deck",0,id,owner,Checks.CreatureInYourDeck)
			closeDeck(owner)
			if(ch>=0) then
				moveCard(ch,ZONE_HAND)
			end
			shuffleDeck(owner)
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Clobber Totem"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 2,

	HandleMessage = function(id)
		Abils.PowerAttacker(id,2000)
		Abils.cantBeBlockedPower(id,5000)
	end
}

Cards["Dimension Gate"] = {
	price_tier = 1,
	shieldtrigger = 1,

	OnCast = function(id)
		local owner = getCardOwner(id)
        openDeck(owner)
	    local ch = createChoice("Choose a creature in your deck",0,id,owner,Checks.CreatureInYourDeck)
        closeDeck(owner)
	    if(ch>=0) then
            moveCard(ch,ZONE_HAND)
        end
		shuffleDeck(owner)
        Functions.EndSpell(id)
	end
}

Cards["Factory Shell Q"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local func = function(id)
			local summon = function(id)
				local owner = getCardOwner(id)
				openDeck(owner)
				local ch = createChoice("Choose a creature in your deck",0,id,owner,Checks.SurvivorInYourDeck)
				closeDeck(owner)
				if(ch>=0) then
					moveCard(ch,ZONE_HAND)
				end
				shuffleDeck(owner)
			end
			Abils.onSummon(id,summon)
		end
		Abils.Survivor(id,func)
	end
}

Cards["Faerie Life"] = {
	price_tier = 1,
	shieldtrigger = 1,

	OnCast = function(id)
		Functions.moveTopCardsFromDeck(getCardOwner(id), ZONE_MANA, 1)
		Functions.EndSpell(id)
	end
}

Cards["Feather Horn, the Tracker"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Forbidding Totem"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id) --test
		if(getCardZone(id)~=ZONE_BATTLE) then
			return
		end
		if(getMessageType()~="get creaturecanattackplayers" and getMessageType()~="get creaturecanattackcreature") then
			return
		end
		local owner = getCardOwner(id)
		local attacker = getMessageInt("attacker")
		if(attacker<0 or getCardOwner(attacker)==owner) then
			return
		end
		local hasAttackableTotem = false
		local size = getZoneSize(owner,ZONE_BATTLE)
		for i=0,(size-1) do
			local creature = getCardAt(owner,ZONE_BATTLE,i)
			if(isCreatureOfRace(creature,"Mystery Totem")==1 and isCardTapped(creature)==1) then
				hasAttackableTotem = true
				break
			end
		end
		if(hasAttackableTotem and getMessageType()=="get creaturecanattackplayers") then
			setMessageInt("canattack",CANATTACK_NO)
		elseif(hasAttackableTotem and getMessageType()=="get creaturecanattackcreature") then
			local defender = getMessageInt("defender")
			if(isCreatureOfRace(defender,"Mystery Totem")==0) then
				setMessageInt("canattack",CANATTACK_NO)
			end
		end
	end
}

Cards["Garabon, the Glider"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.PowerAttacker(id,2000)
	end
}

Cards["Illusory Berry"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Innocent Hunter, Blade of All"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="get creaturecanevolve" and getMessageInt("evobait")==id and getCardZone(id)==ZONE_BATTLE) then
			setMessageInt("canevolve",1)
		end
	end
}

Cards["Living Citadel Vosh"] = {
	price_tier = 3,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Evolution(id,"Colony Beetle")
		local tap = function(id)
			moveTopCardsFromDeck(getCardOwner(id),ZONE_MANA,1)
		end
		Abils.TapAbilityForCiv(id, tap, CIV_NATURE)
	end
}

Cards["Mighty Bandit, Ace of Thieves"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		local tap = function(id)
			local mod = function(cid,mid)
				Abils.bonusPower(cid,5000)
				Abils.destroyModAtEOT(cid,mid)
			end
			local c = createChoice("Choose creature",0,id,getCardOwner(id),Checks.InYourBattle)
			if(c>=0) then
				createModifier(c,mod)
			end
		end
		Abils.TapAbility(id,tap)
	end
}

Cards["Mystic Treasure Chest"] = {
	price_tier = 3,
	shieldtrigger = 0,

	OnCast = function(id)
		local owner = getCardOwner(id)
		local valid = function(cid,sid)
			if(Checks.InYourDeck(cid,sid)==1 and getCardCiv(sid)~=CIV_NATURE) then
				return 1
			end
			return 0
		end
		openDeck(owner)
		local ch = createChoice("Choose a non-nature card in your deck",0,id,owner,valid)
		if(ch>=0) then
			moveCard(ch,ZONE_MANA)
		end
		shuffleDeck(owner)
		closeDeck(owner)
		Functions.EndSpell(id)
	end
}

Cards["Pangaea's Will"] = {
	price_tier = 2,
	shieldtrigger = 1,

	OnCast = function(id) --test
		local ch = createChoice("Choose an opponent's evolution creature",0,id,getCardOwner(id),Checks.EvolutionInOppBattle)
		if(ch>=0) then
			seperateEvolution(ch)
			moveCard(ch,ZONE_MANA)
		end
		Functions.EndSpell(id)
	end
}

Cards["Paradise Horn"] = {
	price_tier = 2,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.PowerAttacker(id,2000)
	end
}

Cards["Slumber Shell"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Trench Scarab"] = {
	price_tier = 1,
	shieldtrigger = 0,
	blocker = 0,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttackPlayers(id)
		Abils.PowerAttacker(id,4000)
	end
}
