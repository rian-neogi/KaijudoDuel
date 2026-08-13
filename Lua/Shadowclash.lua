package.path = package.path .. ';./?.lua;'
require("Lua/Rampage")

Cards["Aeris, Flight Elemental"] = {
	price_tier = 3,
	name = "Aeris, Flight Elemental",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Angel Command",
	cost = 5,

	shieldtrigger = 0,
	blocker = 0,

	power = 9000,
	breaker = 1,

	HandleMessage = function(id)
        Abils.cantAttackPlayers(id)
        if(getMessageType()=="get creaturecanattackcreature") then
		    if(getMessageInt("attacker")==id and getCardCiv(getMessageInt("defender"))==CIV_DARKNESS) then
			    setMessageInt("canattack",CANATTACK_UNTAPPED)
		    end
	    end
	end
}

Cards["Alcadeias, Lord of Spirits"] = {
	price_tier = 4,
	name = "Alcadeias, Lord of Spirits",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Angel Command",
	cost = 6,

	shieldtrigger = 0,
	blocker = 0,

	power = 12500,
	breaker = 2,

	HandleMessage = function(id)
        Abils.Evolution(id,"Angel Command")
        if(getMessageType()=="get cardcancast" and getCardZone(id)==ZONE_BATTLE) then
            local cid = getMessageInt("card")
            if(getCardType(cid)==TYPE_SPELL and getCardCiv(cid)~=CIV_LIGHT) then
                setMessageInt("cancast",0)
            end
        end
	end
}

Cards["Amber Grass"] = {
	price_tier = 1,
	name = "Amber Grass",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Starlight Tree",
	cost = 4,

	shieldtrigger = 1,
	blocker = 0,

	power = 3000,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Ancient Giant"] = {
	price_tier = 3,
	name = "Ancient Giant",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_NATURE,
	race = "Giant",
	cost = 8,

	shieldtrigger = 0,
	blocker = 0,

	power = 9000,
	breaker = 2,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturecanblock") then
		    if(getMessageInt("attacker")==id and getCardCiv(getMessageInt("blocker"))==CIV_DARKNESS) then
			    setMessageInt("canblock",0)
		    end
	    end
	end
}

Cards["Aqua Guard"] = {
	price_tier = 1,
	name = "Aqua Guard",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_WATER,
	race = "Liquid People",
	cost = 1,

	shieldtrigger = 0,
	blocker = 1,

	power = 2000,
	breaker = 1,

	HandleMessage = function(id)
        Abils.cantAttack(id)
	end
}

Cards["Aqua Jolter"] = {
	price_tier = 1,
	name = "Aqua Jolter",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_WATER,
	race = "Liquid People",
	cost = 3,

	shieldtrigger = 1,
	blocker = 0,

	power = 2000,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Aquan"] = {
	price_tier = 3,
	name = "Aquan",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_WATER,
	race = "Liquid People",
	cost = 3,

	shieldtrigger = 1,
	blocker = 0,

	power = 2000,
	breaker = 1,

	HandleMessage = function(id)
		local func = function(id)
			local owner = getCardOwner(id)
			local count = 5 - getZoneSize(owner, ZONE_HAND)
			if(count > 0) then
				drawCards(owner, count)
			end
		end
		Abils.onSummon(id, func)
	end
}

Cards["Astral Warper"] = {
	price_tier = 3,
	name = "Astral Warper",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_WATER,
	race = "Cyber Virus",
	cost = 6,

	shieldtrigger = 0,
	blocker = 0,

	power = 5000,
	breaker = 1,

	HandleMessage = function(id)
        Abils.Evolution(id,"Cyber Virus")
        Abils.drawOnSummon(id,3)
	end
}

Cards["Ballom, Master of Death"] = {
	price_tier = 4,
	name = "Ballom, Master of Death",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Demon Command",
	cost = 8,

	shieldtrigger = 0,
	blocker = 0,

	power = 12000,
	breaker = 2,

	HandleMessage = function(id)
        Abils.Evolution(id,"Demon Command")
        local func = function(id)
            local valid = function(cid,sid)
                if(Checks.InBattle(cid,sid)==1 and getCardCiv(sid)~=CIV_DARKNESS) then
                    return 1
                else
                    return 0
                end
            end
            local func2 = function(cid,sid)
                destroyCreature(sid)
            end
            Functions.execute(id,valid,func2)
        end
        Abils.onSummon(id,func)
	end
}

Cards["Blasto, Explosive Soldier"] = {
	price_tier = 1,
	name = "Blasto, Explosive Soldier",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_FIRE,
	race = "Dragonoid",
	cost = 3,

	shieldtrigger = 0,
	blocker = 0,

	power = 2000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturepower") then
            if(getCardZone(id)==ZONE_BATTLE and getMessageInt("creature")==id) then 
                local owner = getCardOwner(id)
                local size = getZoneSize(owner,ZONE_BATTLE)
                for i=0,(size-1) do
                    local cid = getCardAt(owner,ZONE_BATTLE,i)
                    if(getCardCiv(cid)==CIV_DARKNESS and getCardType(cid)==TYPE_CREATURE) then
                        setMessageInt("power",getMessageInt("power")+2000)
                        break
                    end
                end
            end
        end
	end
}

Cards["Cannon Shell"] = {
	price_tier = 1,
	name = "Cannon Shell",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_NATURE,
	race = "Colony Beetle",
	cost = 4,

	shieldtrigger = 1,
	blocker = 0,

	power = 1000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturepower") then
            if(getMessageInt("creature")==id) then
                local c = getZoneSize(getCardOwner(id),ZONE_SHIELD)
                Abils.bonusPower(id,c*1000)
            end
        end
	end
}

Cards["Chains of Sacrifice"] = {
	price_tier = 3,
	name = "Chains of Sacrifice",
	set = "Shadowclash of Blinding Night",
	type = TYPE_SPELL,
	civilization = CIV_DARKNESS,
	cost = 8,

	shieldtrigger = 0,

	HandleMessage = function(id)
		if(getMessageType()=="get cardaicancast" and getMessageInt("card")==id and
			(getZoneSize(getCardOwner(id),ZONE_BATTLE)==0 or
			getZoneSize(getOpponent(getCardOwner(id)),ZONE_BATTLE)==0)) then
			setMessageInt("cancast",0)
		end
	end,

	OnCast = function(id)
        local ch = createChoice("Destroy an opponent's creature",1,id,getCardOwner(id),Checks.InOppBattle)
        if(ch>=0) then
            destroyCreature(ch)
            local ch2 = createChoice("Destroy an opponent's creature",1,id,getCardOwner(id),Checks.InOppBattle)
            if(ch2>=0) then
                destroyCreature(ch2)
            end
        end
        local ch3 = createChoice("Destroy 1 of your creatures",0,id,getCardOwner(id),Checks.InYourBattle)
        if(ch3>=0) then
            destroyCreature(ch3)
        end
        Functions.EndSpell(id)
	end
}

Cards["Chaotic Skyterror"] = {
	price_tier = 2,
	name = "Chaotic Skyterror",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_FIRE,
	race = "Armored Wyvern",
	cost = 5,

	shieldtrigger = 0,
	blocker = 0,

	power = 4000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturepower" and getCardZone(id)==ZONE_BATTLE) then
            local cid = getMessageInt("creature")
            if(getCardType(cid)==TYPE_CREATURE and getCardZone(cid)==ZONE_BATTLE and isCreatureOfRace(cid,"Demon Command")==1 and getAttacker()==cid) then
                setMessageInt("power",getMessageInt("power")+4000)
            end
        end
        if(getMessageType()=="get creaturebreaker" and getCardZone(id)==ZONE_BATTLE) then
            local cid = getMessageInt("creature")
            if(getCardType(cid)==TYPE_CREATURE and getCardZone(cid)==ZONE_BATTLE and isCreatureOfRace(cid,"Demon Command")==1 and getAttacker()==cid) then
                Abils.Breaker(cid,2)
            end
        end
	end
}

Cards["Clone Factory"] = {
	price_tier = 1,
	name = "Clone Factory",
	set = "Shadowclash of Blinding Night",
	type = TYPE_SPELL,
	civilization = CIV_WATER,
	cost = 3,

	shieldtrigger = 0,

	OnCast = function(id)
        local ch = createChoice("Choose a card in your mana zone",1,id,getCardOwner(id),Checks.InYourMana)
        if(ch>=0) then
            moveCard(ch,ZONE_HAND)
            local ch2 = createChoice("Choose a card in your mana zone",1,id,getCardOwner(id),Checks.InYourMana)
            if(ch2>=0) then
                moveCard(ch2,ZONE_HAND)
            end
        end
	end
}

Cards["Darkpact"] = {
	price_tier = 1,
	name = "Darkpact",
	set = "Shadowclash of Blinding Night",
	type = TYPE_SPELL,
	civilization = CIV_DARKNESS,
	cost = 2,

	shieldtrigger = 0,

	OnCast = function(id)
        local count = 0
        while(true) do
            local ch = createChoice("Choose a card in your mana zone",1,id,getCardOwner(id),Checks.InYourMana)
            if(ch>=0) then
                moveCard(ch,ZONE_GRAVEYARD)
                count = count+1
            end
            if(ch==RETURN_NOVALID or ch==RETURN_BUTTON1) then
                break
            end
        end
        drawCards(getCardOwner(id),count)
        Abils.EndSpell(id)
	end
}

Cards["Dew Mushroom"] = {
	price_tier = 2,
	name = "Dew Mushroom",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_NATURE,
	race = "Balloon Mushroom",
	cost = 3,

	shieldtrigger = 0,
	blocker = 0,

	power = 1000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get cardcost") then
            if(getCardZone(id)==ZONE_BATTLE) then
                local card = getMessageInt("card")
                if(getCardCiv(card)==CIV_DARKNESS) then
                    local cost = getMessageInt("cost")
                    setMessageInt("cost",cost+1)
                end
            end
        end
	end
}

Cards["Doboulgyser, Giant Rock Beast"] = {
	price_tier = 3,
	name = "Doboulgyser, Giant Rock Beast",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_FIRE,
	race = "Rock Beast",
	cost = 6,

	shieldtrigger = 0,
	blocker = 0,

	power = 8000,
	breaker = 2,

	HandleMessage = function(id)
        Abils.Evolution(id,"Rock Beast")
        local valid = function(cid,sid)
            if(Checks.InOppBattle(cid,sid)==1 and getCreaturePower(sid)<=3000) then
                return 1
            else
                return 0
            end
        end
        local func = function(id)
            local ch = createChoice("Choose an opponent's creature",1,id,getCardOwner(id),valid)
            if(ch>=0) then
                destroyCreature(id)
            end
        end
        Abils.onSummon(id,func)
	end
}

Cards["Exploding Cactus"] = {
	price_tier = 1,
	name = "Exploding Cactus",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_NATURE,
	race = "Tree Folk",
	cost = 3,

	shieldtrigger = 0,
	blocker = 0,

	power = 2000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturepower") then
            if(getCardZone(id)==ZONE_BATTLE and getMessageInt("creature")==id) then 
                local owner = getCardOwner(id)
                local size = getZoneSize(owner,ZONE_BATTLE)
                for i=0,(size-1) do
                    local cid = getCardAt(owner,ZONE_BATTLE,i)
                    if(getCardCiv(cid)==CIV_LIGHT and getCardType(cid)==TYPE_CREATURE) then
                        setMessageInt("power",getMessageInt("power")+2000)
                        break
                    end
                end
            end
        end
	end
}

Cards["Fu Reil, Seeker of Storms"] = {
	price_tier = 2,
	name = "Fu Reil, Seeker of Storms",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Mecha Thunder",
	cost = 6,

	shieldtrigger = 0,
	blocker = 0,

	power = 5000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get canuseshieldtrigger") then
            if(getCardCiv(getMessageInt("card"))==CIV_DARKNESS and getCardZone(id)==ZONE_BATTLE) then
                setMessageInt("canuse",0)
            end
        end
	end
}

Cards["Full Defensor"] = {
	price_tier = 2,
	name = "Full Defensor",
	set = "Shadowclash of Blinding Night",
	type = TYPE_SPELL,
	civilization = CIV_LIGHT,
	cost = 2,

	shieldtrigger = 1,

	OnCast = function(id)
        local mod = function(cid,mid)
            Abils.Blocker(cid)
		    Abils.destroyModAtSOT(cid,mid)
        end
		local func = function(cid,sid)
            createModifier(sid,mod)
        end
		Functions.executeForCreaturesInBattle(id,getCardOwner(id),func)
        Functions.EndSpell(id)
	end
}

Cards["Galklife Dragon"] = {
	price_tier = 3,
	name = "Galklife Dragon",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_FIRE,
	race = "Armored Dragon",
	cost = 7,

	shieldtrigger = 0,
	blocker = 0,

	power = 6000,
	breaker = 2,

	HandleMessage = function(id)
        local func = function(id)
            local valid = function(cid,sid)
                if(Checks.InBattle(cid,sid)==1 and getCardCiv(sid)==CIV_LIGHT and getCreaturePower(sid)<=4000) then
                    return 1
                else
                    return 0
                end
            end
            local func2 = function(cid,sid)
                destroyCreature(sid)
            end
            Functions.execute(id,valid,func2)
        end
        Abils.onSummon(id,func)
	end
}

Cards["Gigabolver"] = { --check
	price_tier = 2,
	name = "Gigabolver",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Chimera",
	cost = 4,

	shieldtrigger = 0,
	blocker = 0,

	power = 3000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get canuseshieldtrigger") then
            if(getCardCiv(getMessageInt("card"))==CIV_LIGHT and getCardZone(id)==ZONE_BATTLE) then
                setMessageInt("canuse",0)
            end
        end
	end
}

Cards["Gregoria, Princess of War"] = {
	price_tier = 3,
	name = "Gregoria, Princess of War",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Dark Lord",
	cost = 6,

	shieldtrigger = 0,
	blocker = 0,

	power = 5000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturepower" and getCardZone(id)==ZONE_BATTLE) then
            local cid = getMessageInt("creature")
            if(getCardType(cid)==TYPE_CREATURE and getCardZone(cid)==ZONE_BATTLE and isCreatureOfRace(cid,"Demon Command")==1) then
                setMessageInt("power",getMessageInt("power")+2000)
            end
        end
        if(getMessageType()=="get creatureisblocker" and getCardZone(id)==ZONE_BATTLE) then
            local cid = getMessageInt("creature")
            if(getCardType(cid)==TYPE_CREATURE and getCardZone(cid)==ZONE_BATTLE and isCreatureOfRace(cid,"Demon Command")==1) then
                setMessageInt("isblocker",1)
            end
        end
	end
}

Cards["Gregorian Worm"] = {
	price_tier = 1,
	name = "Gregorian Worm",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Parasite Worm",
	cost = 4,

	shieldtrigger = 1,
	blocker = 0,

	power = 3000,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Gulan Rias, Speed Guardian"] = {
	price_tier = 1,
	name = "Gulan Rias, Speed Guardian",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Guardian",
	cost = 3,

	shieldtrigger = 0,
	blocker = 0,

	power = 2000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturecanblock") then
		    if(getMessageInt("attacker")==id and getCardCiv(getMessageInt("blocker"))==CIV_DARKNESS) then
			    setMessageInt("canblock",0)
		    end
	    end
        if(getMessageType()=="get creaturecanattackcreature") then
		    if(getMessageInt("defender")==id and getCardCiv(getMessageInt("attacker"))==CIV_DARKNESS) then
			    setMessageInt("canattack",CANATTACK_NO)
		    end
	    end
	end
}

Cards["Hunter Cluster"] = {
	price_tier = 2,
	name = "Aeris, Flight Elemental",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_WATER,
	race = "Cyber Cluster",
	cost = 4,

	shieldtrigger = 1,
	blocker = 1,

	power = 1000,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Hydro Hurricane"] = {
	price_tier = 3,
	name = "Hydro Hurricane",
	set = "Shadowclash of Blinding Night",
	type = TYPE_SPELL,
	civilization = CIV_WATER,
	cost = 6,

	shieldtrigger = 0,

	HandleMessage = function(id)
		if(getMessageType()=="get cardaicancast" and getMessageInt("card")==id) then
			local owner=getCardOwner(id)
			local darkness=0
			for i=0,(getZoneSize(owner,ZONE_BATTLE)-1) do
				if(getCardCiv(getCardAt(owner,ZONE_BATTLE,i))==CIV_DARKNESS) then darkness=darkness+1 end
			end
			if(darkness==0 or getZoneSize(getOpponent(owner),ZONE_BATTLE)==0) then
				setMessageInt("cancast",0)
			end
		end
	end,

	OnCast = function(id)
        local owner = getCardOwner(id)
        local size = getZoneSize(owner,ZONE_BATTLE)
        local cl = 0
        local cd = 0
        for i=0,(size-1) do
            local cid = getCardAt(owner,ZONE_BATTLE,i)
            if(getCardType(cid)==TYPE_CREATURE) then
                local civ = getCardCiv(cid)
                if(civ==CIV_LIGHT) then
                    cl = cl+1
                end
                if(civ==CIV_DARKNESS) then
                    cd = cd+1
                end
            end
        end
        for i=1,cl do
            local ch = createChoice("Choose a card in your opponent's mana zone",1,id,getCardOwner(id),Checks.InOppMana)
            if(ch>=0) then
                moveCard(ch,ZONE_HAND)
            end
            if(ch==RETURN_NOVALID or ch==RETURN_BUTTON1) then
                break
            end
        end
        for i=1,cd do
            local ch = createChoice("Choose a card in your opponent's battlezone",1,id,getCardOwner(id),Checks.InOppBattle)
            if(ch>=0) then
                moveCard(ch,ZONE_HAND)
            end
            if(ch==RETURN_NOVALID or ch==RETURN_BUTTON1) then
                break
            end
        end
        Functions.EndSpell(id)
	end
}

Cards["Kamikaze, Chainsaw Warrior"] = {
	price_tier = 1,
	name = "Kamikaze, Chainsaw Warrior",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_FIRE,
	race = "Armorloid",
	cost = 2,

	shieldtrigger = 1,
	blocker = 0,

	power = 1000,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Keeper of the Sunlit Abyss"] = {
	price_tier = 1,
	name = "Keeper of the Sunlit Abyss",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_WATER,
	race = "Cyber Virus",
	cost = 4,

	shieldtrigger = 0,
	blocker = 0,

	power = 2000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturepower" and getCardZone(id)==ZONE_BATTLE) then
            local cid = getMessageInt("creature")
            if(getCardType(cid)==TYPE_CREATURE and getCardZone(cid)==ZONE_BATTLE and (getCardCiv(cid)==CIV_DARKNESS or getCardCiv(cid)==CIV_LIGHT)) then
                setMessageInt("power",getMessageInt("power")+1000)
            end
        end
	end
}

Cards["King Aquakamui"] = {
	price_tier = 3,
	name = "King Aquakamui",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_WATER,
	race = "Leviathan",
	cost = 7,

	shieldtrigger = 0,
	blocker = 0,

	power = 5000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturepower" and getCardZone(id)==ZONE_BATTLE) then
            local cid = getMessageInt("creature")
            if(getCardType(cid)==TYPE_CREATURE and getCardZone(cid)==ZONE_BATTLE and (isCreatureOfRace(cid,"Angel Command")==1 or isCreatureOfRace(cid,"Demon Command")==1)) then
                setMessageInt("power",getMessageInt("power")+2000)
            end
        end
        local func = function(id)
            local ch = createChoiceNoCheck("Return all Angel and Demon Commands to hand?",2,id,getCardOwner(id),Checks.False)
            if(ch==RETURN_BUTTON1) then
                local valid = function(cid,sid)
                    if(Checks.CreatureInYourGraveyard(cid,sid)==1 and (isCreatureOfRace(sid,"Angel Command")==1 or isCreatureOfRace(sid,"Demon Command")==1)) then
                        return 1
                    else
                        return 0
                    end
                end
                local func2 = function(cid,sid)
                    moveCard(sid,ZONE_HAND)
                end
                Functions.execute(id,valid,func2)
            end
        end
        Abils.onSummon(id,func)
	end
}

Cards["Kolon, the Oracle"] = {
	price_tier = 1,
	name = "Kolon, the Oracle",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Light Bringer",
	cost = 4,

	shieldtrigger = 1,
	blocker = 0,

	power = 1000,
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

Cards["Locomotiver"] = {
	price_tier = 2,
	name = "Locomotiver",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Hedrian",
	cost = 4,

	shieldtrigger = 1,
	blocker = 0,

	power = 1000,
	breaker = 1,

	HandleMessage = function(id)
        local summon = function(id)
            discardCardAtRandom(getOpponent(getCardOwner(id)))
        end
        Abils.onSummon(id,summon)
	end
}

Cards["Magmarex"] = {
	price_tier = 2,
	name = "Magmarex",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_FIRE,
	race = "Rock Beast",
	cost = 5,

	shieldtrigger = 1,
	blocker = 0,

	power = 3000,
	breaker = 1,

	HandleMessage = function(id)
        local func = function(id)
            local valid = function(cid,sid)
                if(Checks.InBattle(cid,sid)==1 and getCreaturePower(sid)==1000) then
                    return 1
                else
                    return 0
                end
            end
            local func2 = function(cid,sid)
                destroyCreature(sid)
            end
            Functions.execute(id,valid,func2)
        end
        Abils.onSummon(id,func)
	end
}

Cards["Marinomancer"] = {
	price_tier = 2,
	name = "Marinomancer",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_WATER,
	race = "Cyber Lord",
	cost = 5,

	shieldtrigger = 0,
	blocker = 0,

	power = 2000,
	breaker = 1,

	HandleMessage = function(id)
        local func = function(id)
            local player = getCardOwner(id)
            local size = getZoneSize(player,ZONE_DECK)
            for i=1,3 do
                if(i>size) then
                    break
                end
                local c = getCardAt(player,ZONE_DECK,size-i)
                if(getCardCiv(c)==CIV_LIGHT or getCardCiv(c)==CIV_DARKNESS) then
	                moveCard(c,ZONE_HAND)
                else
                    moveCard(c,ZONE_GRAVEYARD)
                end
            end
        end
        Abils.onSummon(id,func)
	end
}

Cards["Mega Detonator"] = {
	price_tier = 2,
	name = "Mega Detonator",
	set = "Shadowclash of Blinding Night",
	type = TYPE_SPELL,
	civilization = CIV_FIRE,
	cost = 2,

	shieldtrigger = 0,

	OnCast = function(id)
        local count = 0
        local mod = function(cid,mid)
            Abils.Breaker(cid,2)
            Abils.destroyModAtEOT(cid,mid)
        end
        while(true) do
            local ch = createChoice("Choose a card in your mana zone",1,id,getCardOwner(id),Checks.InYourMana)
            if(ch>=0) then
                moveCard(ch,ZONE_GRAVEYARD)
                count = count+1
            end
            if(ch==RETURN_NOVALID or ch==RETURN_BUTTON1) then
                break
            end
        end
        for i=1,count do
            local ch = createChoice("Choose a creature",0,id,getCardOwner(id),Checks.InYourBattle)
            if(ch>=0) then
                createModifier(ch,mod)
            end
        end
        Abils.EndSpell(id)
	end
}

Cards["Milieus, the Daystretcher"] = {
	price_tier = 2,
	name = "Milieus, the Daystretcher",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Berserker",
	cost = 5,

	shieldtrigger = 0,
	blocker = 1,

	power = 2500,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get cardcost") then
            if(getCardZone(id)==ZONE_BATTLE) then
                local card = getMessageInt("card")
                if(getCardCiv(card)==CIV_DARKNESS) then
                    local cost = getMessageInt("cost")
                    setMessageInt("cost",cost+2)
                end
            end
        end
	end
}

Cards["Missile Boy"] = {
	price_tier = 1,
	name = "Missile Boy",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_FIRE,
	race = "Human",
	cost = 3,

	shieldtrigger = 0,
	blocker = 0,

	power = 1000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get cardcost") then
            if(getCardZone(id)==ZONE_BATTLE) then
                local card = getMessageInt("card")
                if(getCardCiv(card)==CIV_LIGHT) then
                    local cost = getMessageInt("cost")
                    setMessageInt("cost",cost+1)
                end
            end
        end
	end
}

Cards["Mist Rias, Sonic Guardian"] = {
	price_tier = 2,
	name = "Mist Rias, Sonic Guardian",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Guardian",
	cost = 5,

	shieldtrigger = 0,
	blocker = 0,

	power = 2000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="post cardmove") then
		    if(getCardZone(id)==ZONE_BATTLE and getCardType(getMessageInt("card"))==TYPE_CREATURE and getMessageInt("to")==ZONE_BATTLE and getMessageInt("card")~=id) then
                local ch = createChoiceNoCheck("Draw a card?",2,id,getCardOwner(id),Checks.False)
                if(ch==RETURN_BUTTON1) then
                    drawCards(getCardOwner(id),1)
                end
            end
        end
	end
}

Cards["Mongrel Man"] = {
	price_tier = 2,
	name = "Mongrel Man",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Hedrian",
	cost = 5,

	shieldtrigger = 0,
	blocker = 0,

	power = 2000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="post creaturedestroy") then
		    if(getCardZone(id)==ZONE_BATTLE and getMessageInt("creature")~=id) then
                local ch = createChoiceNoCheck("Draw a card?",2,id,getCardOwner(id),Checks.False)
                if(ch==RETURN_BUTTON1) then
                    drawCards(getCardOwner(id),1)
                end
            end
        end
	end
}

Cards["Mystic Inscription"] = {
	price_tier = 2,
	name = "Mystic Inscription",
	set = "Shadowclash of Blinding Night",
	type = TYPE_SPELL,
	civilization = CIV_NATURE,
	cost = 6,

	shieldtrigger = 0,

	OnCast = function(id)
        Functions.moveTopCardsFromDeck(getCardOwner(id),ZONE_SHIELD,1)
        Functions.EndSpell(id)
	end
}

Cards["Niofa, Horned Protector"] = {
	price_tier = 3,
	name = "Niofa, Horned Protector",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_NATURE,
	race = "Horned Beast",
	cost = 6,

	shieldtrigger = 0,
	blocker = 0,

	power = 9000,
	breaker = 1,

	HandleMessage = function(id)
        Abils.Evolution(id,"Horned Beast")
        local valid = function(cid,sid)
            if(Checks.CreatureInYourDeck(cid,sid)==1 and getCardCiv(sid)==CIV_NATURE) then
                return 1
            else
                return 0
            end
        end
        local func = function(id)
            local owner = getCardOwner(id)
            openDeck(owner)
	        local preferred = Functions.HighestCostChoice(id,owner,ZONE_DECK,valid)
	        local ch = createChoice("Choose a nature creature in your deck",1,id,owner,valid,preferred)
            closeDeck(owner)
	        if(ch>=0) then
                moveCard(ch,ZONE_HAND)
                shuffleDeck(getCardOwner(ch))
            end
        end
        Abils.onSummon(id,func)
	end
}

Cards["Ouks, Vizier of Restoration"] = {
	price_tier = 3,
	name = "Ouks, Vizier of Restoration",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Initiate",
	cost = 5,

	shieldtrigger = 0,
	blocker = 0,

	power = 1000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="mod creaturedestroy") then
		    if(getMessageInt("creature")==id and getCardZone(id)==ZONE_BATTLE) then
			    setMessageInt("zoneto",ZONE_SHIELD)
		    end
	    end
	end
}

Cards["Photocide, Lord of the Wastes"] = {
	price_tier = 3,
	name = "Photocide, Lord of the Wastes",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Demon Command",
	cost = 5,

	shieldtrigger = 0,
	blocker = 0,

	power = 9000,
	breaker = 1,

	HandleMessage = function(id)
        Abils.cantAttackPlayers(id)
        if(getMessageType()=="get creaturecanattackcreature") then
		    if(getMessageInt("attacker")==id and getCardCiv(getMessageInt("defender"))==CIV_LIGHT) then
			    setMessageInt("canattack",CANATTACK_UNTAPPED)
		    end
	    end
	end
}

Cards["Pippie Kuppie"] = {
	price_tier = 1,
	name = "Pippie Kuppie",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_FIRE,
	race = "Fire Bird",
	cost = 2,

	shieldtrigger = 0,
	blocker = 0,

	power = 1000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturepower" and getCardZone(id)==ZONE_BATTLE) then
            local cid = getMessageInt("creature")
            if(getCardType(cid)==TYPE_CREATURE and getCardZone(cid)==ZONE_BATTLE and isCreatureOfRace(cid,"Armored Dragon")==1) then
                setMessageInt("power",getMessageInt("power")+1000)
            end
        end
	end
}

Cards["Purple Piercer"] = {
	price_tier = 1,
	name = "Purple Piercer",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Brain Jacker",
	cost = 3,

	shieldtrigger = 0,
	blocker = 0,

	power = 2000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturecanblock") then
		    if(getMessageInt("attacker")==id and getCardCiv(getMessageInt("defender"))==CIV_LIGHT) then
			    setMessageInt("canblock",0)
		    end
	    end
        if(getMessageType()=="get creaturecanattackcreature") then
		    if(getMessageInt("defender")==id and getCardCiv(getMessageInt("attacker"))==CIV_LIGHT) then
			    setMessageInt("canblock",CANATTACK_NO)
		    end
	    end
	end
}

Cards["Re Bil, Seeker of Archery"] = {
	price_tier = 3,
	name = "Aeris, Flight Elemental",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Mecha Thunder",
	cost = 7,

	shieldtrigger = 0,
	blocker = 0,

	power = 6000,
	breaker = 2,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturepower" and getCardZone(id)==ZONE_BATTLE) then
            local cid = getMessageInt("creature")
            if(getCardType(cid)==TYPE_CREATURE and getCardZone(cid)==ZONE_BATTLE and getCardCiv(cid)==CIV_LIGHT and getCardOwner(id)==getCardOwner(cid) and cid~=id) then
                setMessageInt("power",getMessageInt("power")+2000)
            end
        end
	end
}

Cards["Rimuel, Cloudbreak Elemental"] = {
	price_tier = 3,
	name = "Rimuel, Cloudbreak Elemental",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Angel Command",
	cost = 8,

	shieldtrigger = 0,
	blocker = 0,

	power = 6000,
	breaker = 2,

	HandleMessage = function(id)
        local valid = function(cid,sid)
            if(Checks.InYourMana(cid,sid)==1 and isCardTapped(sid)==0 and getCardCiv(sid)==CIV_LIGHT) then
                return 1
            else
                return 0
            end
        end
        local func = function(id)
            local c = Functions.count(id,valid)
            for i=1,c do
                local ch = createChoice("Choose an opponent's creature",0,id,getCardOwner(id),Checks.InOppBattle)
                if(ch>=0) then
                    tapCard(ch)
                end
            end
        end
        Abils.onSummon(id,func)
	end
}

Cards["Sarius, Vizier of Suppression"] = {
	price_tier = 1,
	name = "Sarius, Vizier of Suppression",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_LIGHT,
	race = "Initiate",
	cost = 2,

	shieldtrigger = 0,
	blocker = 1,

	power = 3000,
	breaker = 1,

	HandleMessage = function(id)
        Abils.cantAttackPlayers(id)
	end
}

Cards["Screaming Sunburst"] = {
	price_tier = 2,
	name = "Screaming Sunburst",
	set = "Shadowclash of Blinding Night",
	type = TYPE_SPELL,
	civilization = CIV_LIGHT,
	cost = 3,

	shieldtrigger = 0,

	OnCast = function(id)
        local owner = getCardOwner(id)
		local opp = getOpponent(owner)
		local size = getZoneSize(opp,ZONE_BATTLE)-1
		for i=0,size,1 do
            local cid = getCardAt(opp,ZONE_BATTLE,i)
            if(getCardCiv(cid)~=CIV_LIGHT) then
                tapCard(getCardAt(opp,ZONE_BATTLE,i))
            end
		end
        Functions.EndSpell(id)
	end
}

Cards["Shadow Moon, Cursed Shade"] = {
	price_tier = 2,
	name = "Shadow Moon, the Cursed Shade",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Ghost",
	cost = 4,

	shieldtrigger = 0,
	blocker = 0,

	power = 3000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturepower" and getCardZone(id)==ZONE_BATTLE) then
            local cid = getMessageInt("creature")
            if(getCardType(cid)==TYPE_CREATURE and getCardZone(cid)==ZONE_BATTLE and getCardCiv(cid)==CIV_DARKNESS and getCardOwner(id)==getCardOwner(cid) and cid~=id) then
                setMessageInt("power",getMessageInt("power")+2000)
            end
        end
	end
}

Cards["Skeleton Thief, the Revealer"] = {
	price_tier = 1,
	name = "Skeleton Thief, the Revealer",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Living Dead",
	cost = 4,

	shieldtrigger = 0,
	blocker = 0,

	power = 2000,
	breaker = 1,

	HandleMessage = function(id)
        local func = function(id)
            local valid = function(cid,sid)
                if(Checks.CreatureInYourGraveyard(cid,sid)==1 and isCreatureOfRace(sid,"Living Dead")==1) then
                    return 1
                else
                    return 0
                end
            end
            for i=1,count do
                local ch = createChoice("Select a Living Dead in your graveyard",0,id,getCardOwner(id),valid)
                if(ch>=0) then
                    moveCard(ch,ZONE_HAND)
                end
            end
        end
        Abils.onSummon(id,func)
	end
}

Cards["Smile Angler"] = {
	price_tier = 2,
	name = "Smile Angler",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_WATER,
	race = "Gel Fish",
	cost = 6,

	shieldtrigger = 0,
	blocker = 0,

	power = 3000,
	breaker = 1,

	HandleMessage = function(id)
        local func = function(id)
            local ch = createChoice("Choose a card in your opponent's mana zone",1,id,getCardOwner(id),Checks.InOppMana)
            if(ch>=0) then
                moveCard(ch,ZONE_HAND)
            end
        end
        Abils.onAttack(id,func)
	end
}

Cards["Soul Gulp"] = {
	price_tier = 1,
	name = "Soul Gulp",
	set = "Shadowclash of Blinding Night",
	type = TYPE_SPELL,
	civilization = CIV_DARKNESS,
	cost = 4,

	shieldtrigger = 0,

	HandleMessage = function(id)
		if(getMessageType()=="get cardaicancast" and getMessageInt("card")==id) then
			local owner=getCardOwner(id)
			local hasLightCreature=false
			for i=0,(getZoneSize(getOpponent(owner),ZONE_BATTLE)-1) do
				if(getCardCiv(getCardAt(getOpponent(owner),ZONE_BATTLE,i))==CIV_LIGHT) then
					hasLightCreature=true
					break
				end
			end
			if(not hasLightCreature or getZoneSize(getOpponent(owner),ZONE_HAND)==0) then
				setMessageInt("cancast",0)
			end
		end
	end,

	OnCast = function(id)
        local valid = function(cid,sid)
            if(Checks.InOppBattle(cid,sid)==1 and getCardCiv(sid)==CIV_LIGHT) then
                return 1
            else
                return 0
            end
        end
        local c = Functions.count(id,valid)
        local opp = getOpponent(getCardOwner(id))
        for i=1,c do
            local ch = createChoice("Choose a card to discard",1,id,opp,Checks.InOppHand)
            if(ch>=0) then
                discardCard(ch)
            end
        end
	end
}

Cards["Supporting Tulip"] = {
	price_tier = 1,
	name = "Supporting Tulip",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_NATURE,
	race = "Tree Folk",
	cost = 5,

	shieldtrigger = 0,
	blocker = 0,

	power = 4000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get creaturepower" and getCardZone(id)==ZONE_BATTLE) then
            local cid = getMessageInt("creature")
            if(getCardType(cid)==TYPE_CREATURE and getCardZone(cid)==ZONE_BATTLE and isCreatureOfRace(cid,"Angel Command")==1 and getAttacker()==cid) then
                setMessageInt("power",getMessageInt("power")+4000)
            end
        end
	end
}

Cards["Sword of Benevolent Life"] = {
	price_tier = 2,
	name = "Sword of Benevolent Life",
	set = "Shadowclash of Blinding Night",
	type = TYPE_SPELL,
	civilization = CIV_NATURE,
	cost = 2,

	shieldtrigger = 0,

	OnCast = function(id)
        local valid = function(cid,sid)
            if(Checks.InYourBattle(cid,sid)==1 and getCardCiv(sid)==CIV_LIGHT) then
                return 1
            else
                return 0
            end
        end
        local mod = function(cid,mid)
            if(getMessageType()=="get creaturepower") then
		        if(getMessageInt("creature")==cid) then
                    local count = Functions.count(id,valid)
			        setMessageInt("power",getMessageInt("power")+1000*count)
		        end
	        end
        end
        local func = function(cid,sid)
            createModifier(sid,mod)
        end
        Functions.executeForCreaturesInBattle(id,getCardOwner(id),func)
        Functions.EndSpell(id)
	end
}

Cards["Sword of Malevolent Death"] = {
	price_tier = 2,
	name = "Sword of Malevolent Death",
	set = "Shadowclash of Blinding Night",
	type = TYPE_SPELL,
	civilization = CIV_FIRE,
	cost = 4,

	shieldtrigger = 0,

	OnCast = function(id)
        local valid = function(cid,sid)
            if(Checks.InYourMana(cid,sid)==1 and getCardCiv(sid)==CIV_DARKNESS) then
                return 1
            else
                return 0
            end
        end
        local mod = function(cid,mid)
            if(getMessageType()=="get creaturepower") then
		        if(getMessageInt("creature")==cid and getAttacker()==cid) then
                    local count = Functions.count(id,valid)
			        setMessageInt("power",getMessageInt("power")+1000*count)
		        end
	        end
        end
        local func = function(cid,sid)
            createModifier(sid,mod)
        end
        Functions.executeForCreaturesInBattle(id,getCardOwner(id),func)
        Functions.EndSpell(id)
	end
}

Cards["Three-Eyed Dragonfly"] = {
	price_tier = 2,
	name = "Three-Eyed Dragonfly",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_NATURE,
	race = "Giant Insect",
	cost = 5,

	shieldtrigger = 0,
	blocker = 0,

	power = 4000,
	breaker = 1,

	HandleMessage = function(id)
        local func = function(id)
            local valid = function(cid,sid)
                if(Checks.InYourBattle(cid,sid)==1 and cid~=sid) then
                    return 1
                else
                    return 0
                end
            end
            local ch = createChoice("Choose a creature in your battlezone",1,id,getCardOwner(id),valid)
            if(ch>=0) then
                destroyCreature(ch)
                local mod = function(cid,mid)
                    Abils.PowerAttacker(cid,2000)
                    Abils.Breaker(cid,2)
		            Abils.destroyModAtEOT(cid,mid)
                end
                createModifier(id,mod)
            end
        end
        Abils.onAttack(id,func)
	end
}

Cards["Torcon"] = {
	price_tier = 1,
	name = "Torcon",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_NATURE,
	race = "Beast Folk",
	cost = 2,

	shieldtrigger = 1,
	blocker = 0,

	power = 1000,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Trox, General of Destruction"] = {
	price_tier = 4,
	name = "Trox, General of Destruction",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Demon Command",
	cost = 7,

	shieldtrigger = 0,
	blocker = 0,

	power = 6000,
	breaker = 2,

	HandleMessage = function(id)
        local func = function(id)
            local owner = getCardOwner(id)
            local size = getZoneSize(owner,ZONE_BATTLE)
            for i=0,(size-1) do
                local cid = getCardAt(owner,ZONE_BATTLE,i)
                if(getCardCiv(cid)==CIV_DARKNESS and getCardType(cid)==TYPE_CREATURE and cid~=id) then
                    discardCardAtRandom(getOpponent(owner))
                end
            end
        end
        Abils.onSummon(id,func)
	end
}

Cards["Volcano Smog, Deceptive Shade"] = {
	price_tier = 2,
	name = "Volcano Smog, Deceptive Shade",
	set = "Shadowclash of Blinding Night",
	type = TYPE_CREATURE,
	civilization = CIV_DARKNESS,
	race = "Ghost",
	cost = 6,

	shieldtrigger = 0,
	blocker = 0,

	power = 5000,
	breaker = 1,

	HandleMessage = function(id)
        if(getMessageType()=="get cardcost") then
            if(getCardZone(id)==ZONE_BATTLE) then
                local card = getMessageInt("card")
                if(getCardCiv(card)==CIV_LIGHT) then
                    local cost = getMessageInt("cost")
                    setMessageInt("cost",cost+2)
                end
            end
        end
	end
}

Cards["Whisking Whirlwind"] = {
	price_tier = 1,
	name = "Whisking Whirlwind",
	set = "Shadowclash of Blinding Night",
	type = TYPE_SPELL,
	civilization = CIV_LIGHT,
	cost = 1,

	shieldtrigger = 0,

	OnCast = function(id)
        local mod = function(cid,mid)
            if(getMessageType()=="pre endturn") then
		        if(getMessageInt("player")==getCardOwner(cid) and getCardZone(cid)==ZONE_BATTLE and isCardTapped(cid)==1) then
                    untapCard(cid)
                end
            end
		    Abils.destroyModAtEOT(cid,mid)
        end
		local func = function(cid,sid)
            createModifier(sid,mod)
        end
		Functions.executeForCreaturesInBattle(id,getCardOwner(id),func)
        Functions.EndSpell(id)
	end
}
