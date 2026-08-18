package.path = package.path .. ';./?.lua;'
require("Lua/BaseSet")

Cards["Amnis, Holy Elemental"] = {
	price_tier = 3,
    name = "Amnis, Holy Elemental",
    set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Angel Command",
	cost = 7,

	shieldtrigger = 0,
	blocker = 1,

	power = 5000,
	breaker = 1,

    HandleMessage = function(id)
        if(getMessageType()=="get creaturecanblock") then
		    if(getMessageInt("blocker")==id) then
                if(not cardHasCivilization(getMessageInt("attacker"),CIV_DARKNESS)) then
			        setMessageInt("canblock",0)
                end
		    end
	    end
        local mod = function(cid,mid)
            if(getMessageType()=="mod creaturedestroy") then
		        if(getMessageInt("creature")==cid and getCardZone(cid)==ZONE_BATTLE) then
			        setMessageInt("msgContinue",0)
                    destroyModifier(cid,mid)
		        end
	        end
        end
        if(getMessageType()=="pre creaturebattle") then
		    if((getMessageInt("attacker")==id and cardHasCivilization(getMessageInt("defender"),CIV_DARKNESS)) or (getMessageInt("defender")==id and cardHasCivilization(getMessageInt("attacker"),CIV_DARKNESS))) then
			    if(getCardZone(id)==ZONE_BATTLE) then
				    createModifier(id,mod)
			    end
		    end
	    end
    end
}

Cards["Armored Groblav"] = {
	price_tier = 3,
	name = "Armored Groblav",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_FIRE,
	race = "Human",
	cost = 5,

	shieldtrigger = 0,
	blocker = 0,

	power = 6000,
	breaker = 2,

	HandleMessage = function(id)
        Abils.Evolution(id,"Human")
	    if(getMessageType()=="get creaturepower") then
		    if(getMessageInt("creature")==id and getAttacker()==id) then
				local player = getCardOwner(id)
                local size = getZoneSize(player,ZONE_BATTLE)
                local count = 0
                for i=0,(size-1) do
                    local sid = getCardAt(player,ZONE_BATTLE,i)
                    if(getCardType(sid)==TYPE_CREATURE and cardHasCivilization(sid,CIV_FIRE) and id~=sid) then
                        count = count+1
                    end
                end
                size = getZoneSize(getOpponent(player),ZONE_BATTLE)
                for i=0,(size-1) do
                    local sid = getCardAt(getOpponent(player),ZONE_BATTLE,i)
                    if(getCardType(sid)==TYPE_CREATURE and cardHasCivilization(sid,CIV_FIRE) and id~=sid) then
                        count = count+1
                    end
                end
			    setMessageInt("power",getMessageInt("power")+(count*1000))
		    end
        end
	end
}

Cards["Cranium Clamp"] = {
	price_tier = 3,
	name = "Cranium Clamp",
	set = "Promo",
	type = TYPE_SPELL,
	civilization = CIV_DARKNESS,
	cost = 4,

	shieldtrigger = 0,

	HandleMessage = function(id)
		Abils.AiCanCastIfOpponentHasHand(id)
	end,

	OnCast = function(id)
		local opponent = getOpponent(getCardOwner(id))
		for i=1,2 do
			local chosen = createChoice("Choose a card from your hand to discard",0,id,opponent,Checks.InOppHand)
			if(chosen>=0) then
				discardCard(chosen)
			end
		end
	end
}

Cards["Cryptic Totem"] = {
	price_tier = 3,
	name = "Cryptic Totem",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_NATURE,
	race = "Mystery Totem",
	cost = 6,

	shieldtrigger = 0,
	blocker = 0,

	power = 6000,
	breaker = 2,

	HandleMessage = function(id)
		if(getMessageType()=="get canuseshieldtrigger" and
			getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==1) then
			local trigger=getMessageInt("card")
			if(getCardOwner(trigger)~=getCardOwner(id)) then
				setMessageInt("canuse",0)
			end
		end
	end
}

Cards["Gigagrax"] = {
	price_tier = 3,
	name = "Gigagrax",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Chimera",
	cost = 8,

	shieldtrigger = 0,
	blocker = 0,

	power = 5000,
	breaker = 1,

	HandleMessage = function(id)
	    if(getMessageType()=="post creaturedestroy") then
		    if(getMessageInt("creature")==id) then
                local ch = createChoice("Destroy an opponent's creature",1,id,getCardOwner(id),Checks.InOppBattle)
                if(ch>=0) then
                    destroyCard(ch)
                end
		    end
        end
	end
}

Cards["Loth Rix, The Iridescent"] = {
	price_tier = 3,
	name = "Loth Rix, The Iridescent",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Guardian",
	cost = 6,

	shieldtrigger = 0,
	blocker = 0,

	power = 4000,
	breaker = 1,

	HandleMessage = function(id)
        Abils.Evolution(id,"Guardian")
	    local func = function(id)
            Functions.moveTopCardsFromDeck(getCardOwner(id),ZONE_SHIELD,1)
        end
        Abils.onSummon(id,func)
	end
}

Cards["Neve, the Leveler"] = {
	price_tier = 3,
	name = "Neve, the Leveler",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_NATURE,
	race = "Snow Faerie",
	cost = 6,

	shieldtrigger = 0,
	blocker = 0,

	power = 4000,
	breaker = 1,

	HandleMessage = function(id)
	    local func = function(id)
            local owner = getCardOwner(id)
            local c1 = Functions.countCreaturesInBattle(owner)
            local c2 = Functions.countCreaturesInBattle(getOpponent(owner))
            if(c2>c1) then
                openDeck(owner)
                for i=1,(c2-c1) do
                    local preferred = Functions.HighestCostChoice(id,owner,ZONE_DECK,Checks.CreatureInYourDeck)
                    local ch = createChoice("Choose a creature in your deck",1,id,owner,Checks.CreatureInYourDeck,preferred)
	                if(ch>=0) then
                        moveCard(ch,ZONE_HAND)
                        shuffleDeck(getCardOwner(ch))
                    end
                    if(ch==RETURN_BUTTON1 or ch==RETURN_NOVALID) then
                        break
                    end
                end
                closeDeck(owner)
            end
        end
        Abils.onSummon(id,func)
	end
}

Cards["Olgate, Nightmare Samurai"] = {
	price_tier = 3,
	name = "Olgate, Nightmare Samurai",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Demon Command",
	cost = 7,

	shieldtrigger = 0,
	blocker = 0,

	power = 6000,
	breaker = 2,

	HandleMessage = function(id)
	    if(getMessageType()=="post creaturedestroy") then
            if(getCardOwner(id)==getCardOwner(getMessageInt("creature")) and getCardZone(id)==ZONE_BATTLE and isCardTapped(id)==1 and getMessageInt("creature")~=id) then
                local ch = createChoiceNoCheck("Untap this creature?",2,id,getCardOwner(id),Checks.False)
			    if(ch==RETURN_BUTTON1) then
                    untapCard(id)
                end
            end
        end
	end
}

Cards["Star-Cry Dragon"] = {
	price_tier = 3,
	name = "Star-Cry Dragon",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_FIRE,
	race = "Armored Dragon",
	cost = 7,

	shieldtrigger = 0,
	blocker = 0,

	power = 8000,
	breaker = 2,

	HandleMessage = function(id)
	    if(getMessageType()=="get creaturepower") then
            local cid = getMessageInt("creature")
            if(getCardOwner(id)==getCardOwner(cid) and isCreatureOfRace(cid,"Armored Dragon")==1 and getCardZone(cid)==ZONE_BATTLE) then
                Abils.bonusPower(cid,3000)
            end
        end
	end
}

Cards["Twister Fish"] = {
	price_tier = 3,
	name = "Twister Fish",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_WATER,
	race = "Gel Fish",
	cost = 5,

	shieldtrigger = 1,
	blocker = 0,

	power = 3000,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Velyrika Dragon"] = {
	price_tier = 3,
	name = "Velyrika Dragon",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_FIRE,
	race = "Armored Dragon",
	cost = 7,

	shieldtrigger = 0,
	blocker = 0,

	power = 7000,
	breaker = 2,

	HandleMessage = function(id)
        local func = function(id)
            local func2 = function(cid,sid)
                if(getCardOwner(sid)==getCardOwner(cid) and getCardZone(sid)==ZONE_DECK and getCardType(sid)==TYPE_CREATURE and isCreatureOfRace(sid,"Armored Dragon")==1) then
		            return 1
	            else
		            return 0
	            end
            end
            local owner = getCardOwner(id)
            openDeck(owner)
	        local preferred = Functions.HighestCostChoice(id,owner,ZONE_DECK,func2)
	        local ch = createChoice("Choose an Armored Dragon in your deck",0,id,owner,func2,preferred)
            closeDeck(owner)
	        if(ch>=0) then
                moveCard(ch,ZONE_HAND)
                shuffleDeck(getCardOwner(ch))
            end
        end
        Abils.onSummon(id,func)
	end
}

Cards["Uberdragon Zaschack"] = {
	price_tier = 3,
	name = "Uberdragon Zaschack",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_FIRE,
	race = "Armored Dragon",
	cost = 9,

	shieldtrigger = 0,
	blocker = 0,

	power = 11000,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Evolution(id,"Armored Dragon")
		if(getMessageType()=="get creaturebreaker" and getMessageInt("creature")==id and getCardZone(id)==ZONE_BATTLE) then
			local owner = getCardOwner(id)
			local size = getZoneSize(owner,ZONE_BATTLE)
			local count = 0
			for i=0,(size-1) do
				local cid = getCardAt(owner,ZONE_BATTLE,i)
				if(cid~=id and getCardType(cid)==TYPE_CREATURE and isCreatureOfRace(cid,"Armored Dragon")==1) then
					count = count+1
				end
			end
			setMessageInt("breaker",getMessageInt("breaker")+count)
		end
	end
}

Cards["Angry Maple"] = {
	price_tier = 3,
	name = "Angry Maple",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_NATURE,
	race = "Tree Folk",
	cost = 3,

	shieldtrigger = 0,
	blocker = 0,

	power = 1000,
	breaker = 1,

	HandleMessage = function(id)
		Abils.PowerAttacker(id,4000)
	end
}

Cards["Super Dragon Machine Dolzark"] = {
	price_tier = 3,
	name = "Super Dragon Machine Dolzark",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_FIRE,
	race = "Armored Dragon/Earth Dragon",
	cost = 6,

	shieldtrigger = 0,
	blocker = 0,

	power = 7000,
	breaker = 2,

	HandleMessage = function(id)
		local owner = getCardOwner(id)
		if(getMessageType()=="post cardmove" and getMessageInt("card")==id and
			getMessageInt("to")==ZONE_MANA) then
			tapCard(id)
		elseif(getMessageType()=="post creatureattack" and getCardZone(id)==ZONE_BATTLE) then
			local attacker = getMessageInt("attacker")
			if(attacker~=id and getCardOwner(attacker)==owner and
				getCardType(attacker)==TYPE_CREATURE and isCreatureOfRace(attacker,"Dragon")==1) then
				local valid = function(cid,sid)
					return getCardOwner(sid)~=owner and getCardZone(sid)==ZONE_BATTLE and
						getCardType(sid)==TYPE_CREATURE and getCreaturePower(sid)<=5000 and 1 or 0
				end
				local chosen = createChoice("Choose an opponent's creature with power 5000 or less",2,id,owner,valid)
				if(chosen>=0) then moveCard(chosen,ZONE_MANA) end
			end
		end
	end
}

Cards["Dyno Mantis, the Mightspinner"] = {
	price_tier = 3,
	name = "Dyno Mantis, the Mightspinner",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_NATURE,
	race = "Giant Insect",
	cost = 5,

	shieldtrigger = 0,
	blocker = 0,

	power = 7000,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id,"Giant Insect")
		if(getMessageType()=="get creaturebreaker" and getCardZone(id)==ZONE_BATTLE) then
			local creature = getMessageInt("creature")
			if(creature~=id and getCardOwner(creature)==getCardOwner(id) and
				getCardZone(creature)==ZONE_BATTLE and getCardType(creature)==TYPE_CREATURE and
				getCreaturePower(creature)>=5000) then
				setMessageInt("breaker",getMessageInt("breaker")+1)
			end
		end
	end
}

Cards["Giliam, the Tormentor"] = {
	price_tier = 3,
	name = "Giliam, the Tormentor",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Demon Command",
	cost = 7,

	shieldtrigger = 0,
	blocker = 1,

	power = 5000,
	breaker = 1,

	HandleMessage = function(id)
		if(getMessageType()=="get creaturecanblock" and getCardZone(id)==ZONE_BATTLE and
			getMessageInt("blocker")==id and
			not cardHasCivilization(getMessageInt("attacker"),CIV_LIGHT)) then
			setMessageInt("canblock",0)
		end

		if(getMessageType()=="pre creaturebattle" and getCardZone(id)==ZONE_BATTLE) then
			local attacker = getMessageInt("attacker")
			local defender = getMessageInt("defender")
			local opponent = attacker==id and defender or defender==id and attacker or -1
			if(opponent>=0 and cardHasCivilization(opponent,CIV_LIGHT) and
				getCreaturePower(id)<getCreaturePower(opponent)) then
				local protect = function(cid,mid)
					if(getMessageType()=="mod creaturedestroy" and getMessageInt("creature")==cid and
						getCardZone(cid)==ZONE_BATTLE) then
						setMessageInt("msgContinue",0)
						destroyModifier(cid,mid)
					end
				end
				createModifier(id,protect)
			end
		end
	end
}

Cards["Q-tronic Omnistrain"] = {
	price_tier = 3,
	name = "Q-Tronic Omnistrain",
	set = "Promo",
	type = TYPE_CREATURE,
	civilization = CIV_NATURE,
	race = "Survivor",
	cost = 6,

	shieldtrigger = 1,
	blocker = 0,

	power = 3000,
	breaker = 1,

	HandleMessage = function(id)
		Abils.Evolution(id,"Survivor")
		if(getMessageType()=="get creaturerace" and getCardZone(id)==ZONE_BATTLE) then
			local creature = getMessageInt("creature")
			if(getCardOwner(creature)==getCardOwner(id) and getCardZone(creature)==ZONE_BATTLE and
				getCardType(creature)==TYPE_CREATURE) then
				local race = getMessageString("race")
				if(string.find(race,"Survivor",1,true)==nil) then
					setMessageString("race",race.."/Survivor")
				end
			end
		end
	end
}
