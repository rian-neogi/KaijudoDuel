package.path = package.path .. ';./?.lua;'
require("Lua/Common")

Cards["Hollow Soldier"] = {
	price_tier = 5,
	name = "Hollow Soldier",
	set = "Hollow",
	type = TYPE_CREATURE,
	civilization = CIV_HOLLOW,
	race = "Hollow",
	cost = 1,

	shieldtrigger = 0,
	blocker = 0,

	power = 3000,
	breaker = 1,

	HandleMessage = function(id)
	end
}

Cards["Hollow Hulcus"] = {
	price_tier = 5,
	name = "Hollow Hulcus",
	set = "Hollow",
	type = TYPE_CREATURE,
	civilization = CIV_HOLLOW,
	race = "Hollow",
	cost = 3,

	shieldtrigger = 0,
	blocker = 0,

	power = 3000,
	breaker = 1,

	HandleMessage = function(id)
		Abils.drawOnSummon(id,2)
	end
}

Cards["Hollow Tribe"] = {
	price_tier = 5,
	name = "Hollow Tribe",
	set = "Hollow",
	type = TYPE_CREATURE,
	civilization = CIV_HOLLOW,
	race = "Hollow",
	cost = 3,

	shieldtrigger = 0,
	blocker = 0,

	power = 3000,
	breaker = 1,

	HandleMessage = function(id)
		Abils.onSummon(id,function(id)
			Functions.moveTopCardsFromDeck(getCardOwner(id),ZONE_MANA,2)
		end)
	end
}

Cards["Hollow Knight"] = {
	price_tier = 5,
	name = "Hollow Knight",
	set = "Hollow",
	type = TYPE_CREATURE,
	civilization = CIV_HOLLOW,
	race = "Hollow",
	cost = 3,

	shieldtrigger = 0,
	blocker = 0,

	power = 6000,
	breaker = 2,

	HandleMessage = function(id)
	end
}

Cards["Hollow Dragon"] = {
	price_tier = 5,
	name = "Hollow Dragon",
	set = "Hollow",
	type = TYPE_CREATURE,
	civilization = CIV_HOLLOW,
	race = "Hollow",
	cost = 7,

	shieldtrigger = 0,
	blocker = 0,

	power = 15000,
	breaker = 3,

	HandleMessage = function(id)
	end
}

Cards["Hollow Guardian"] = {
	price_tier = 5,
	name = "Hollow Guardian",
	set = "Hollow",
	type = TYPE_CREATURE,
	civilization = CIV_HOLLOW,
	race = "Hollow",
	cost = 2,

	shieldtrigger = 0,
	blocker = 1,

	power = 5000,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttack(id)
	end
}

Cards["Hollow Angel"] = {
	price_tier = 5,
	name = "Hollow Angel",
	set = "Hollow",
	type = TYPE_CREATURE,
	civilization = CIV_HOLLOW,
	race = "Hollow",
	cost = 6,

	shieldtrigger = 0,
	blocker = 1,

	power = 10000,
	breaker = 1,

	HandleMessage = function(id)
		Abils.cantAttack(id)
		Abils.untapAfterBlock(id)
	end
}

Cards["Hollow Demon"] = {
	price_tier = 5,
	name = "Hollow Demon",
	set = "Hollow",
	type = TYPE_CREATURE,
	civilization = CIV_HOLLOW,
	race = "Hollow",
	cost = 6,

	shieldtrigger = 0,
	blocker = 0,

	power = 6000,
	breaker = 2,

	HandleMessage = function(id)
		Abils.AiRemovalTarget(id,Checks.InOppBattle)
		Abils.onSummon(id,function(id)
			local creature = createChoice("Choose an opponent's creature to destroy",0,
				id,getCardOwner(id),Checks.InOppBattle)
			if(creature>=0) then destroyCreature(creature) end
		end)
	end
}

Cards["Hollow Giant"] = {
	price_tier = 5,
	name = "Hollow Giant",
	set = "Hollow",
	type = TYPE_CREATURE,
	civilization = CIV_HOLLOW,
	race = "Hollow",
	cost = 5,

	shieldtrigger = 0,
	blocker = 0,

	power = 7000,
	breaker = 2,

	HandleMessage = function(id)
		Abils.onSummon(id,function(id)
			for i=1,2 do
				local mana = createChoice("Choose a card in your opponent's mana zone",0,
					id,getCardOwner(id),Checks.InOppMana)
				if(mana>=0) then destroyMana(mana) end
			end
		end)
	end
}

Cards["Pure Hollow"] = {
	price_tier = 5,
	name = "Pure Hollow",
	set = "Hollow",
	type = TYPE_CREATURE,
	civilization = CIV_HOLLOW,
	race = "Hollow",
	cost = 7,

	shieldtrigger = 0,
	blocker = 0,

	power = 11000,
	breaker = 2,

	HandleMessage = function(id)
		Abils.Evolution(id,"Hollow")
		if(getCardZone(id)~=ZONE_BATTLE) then return end
		local message = getMessageType()
		if(message~="get creaturepower" and message~="get creaturebreaker") then return end
		local creature = getMessageInt("creature")
		if(creature~=id and getCardOwner(creature)==getCardOwner(id) and
			getCardZone(creature)==ZONE_BATTLE and isCreatureOfRace(creature,"Hollow")==1) then
			if(message=="get creaturepower") then
				setMessageInt("power",getMessageInt("power")+4000)
			else
				setMessageInt("breaker",getMessageInt("breaker")+1)
			end
		end
	end
}

Cards["Pit of Hollows"] = {
	price_tier = 5,
	name = "Pit of Hollows",
	set = "Hollow",
	type = TYPE_SPELL,
	civilization = CIV_HOLLOW,
	cost = 6,
	shieldtrigger = 1,

	HandleMessage = function(id)
	end,

	OnCast = function(id)
		local opponent = getOpponent(getCardOwner(id))
		discardCardAtRandom(opponent)

		local creature = getRandomCardInZone(opponent,ZONE_BATTLE)
		if(creature>=0) then destroyCreature(creature) end

		local mana = getRandomCardInZone(opponent,ZONE_MANA)
		if(mana>=0) then destroyMana(mana) end
		Functions.EndSpell(id)
	end
}
