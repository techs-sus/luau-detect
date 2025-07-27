local a = 10

local function a(eventName, eventArgs)
	local eventNameStr = tostring(eventArgs, "ae")
	return function()
		return eventArgs, eventName, eventNameStr
	end
end
