Cards = {}

package.path = package.path .. ';./?.lua;'
require("Lua/AIParams")
require("Lua/Eternal Arms")
require("Lua/Eternal Wave")
require("Lua/Eternal Vortex")

loadCards = function()
	for k,v in pairs(Cards) do loadcard(k,v.set) end
end

getCardData = function(card,value)
	return Cards[card].value
end
