local memreader = require('memreader')
local process = nil

local activated         = false
local tick_countup      = 0
local source_settings   = nil
local source_name       = ""
local eldenring_igt     = 0
local eldenring_in_game = false
local format_str        = "IGT %s"


local address_table = {
	event_flag_man_addr = 0,
	game_data_man_addr = 0,
	menu_man_imp = 0,
	screen_state_offset = 0,
}

local function get_offset_table()
	local version = process:version()
	if version == nil then
		return
	end
	local ver = version.file
	-- print('> Version: ' .. ver.major .. '.' .. ver.minor .. '.' .. ver.build)
	if ver.major == 1 and ver.minor == 2 and ver.build == 0 then
		address_table.event_flag_man_addr = 0x3c526e8
		address_table.game_data_man_addr = 0x3c481b8
		address_table.menu_man_imp = 0x3c55b30
		address_table.screen_state_offset = 0x718
	elseif ver.major == 1 and ver.minor == 2 and ver.build == 1 then
		address_table.event_flag_man_addr = 0x3c52708
		address_table.game_data_man_addr = 0x3c481d8
		address_table.menu_man_imp = 0x3c55b50
		address_table.screen_state_offset = 0x718
	elseif ver.major == 1 and ver.minor == 2 and ver.build == 2 then
		address_table.event_flag_man_addr = 0x3c52728
		address_table.game_data_man_addr = 0x3c481f8
		address_table.menu_man_imp = 0x3c55b70
		address_table.screen_state_offset = 0x718
	elseif ver.major == 1 and ver.minor == 2 and ver.build == 3 then
		address_table.event_flag_man_addr = 0x3c55748
		address_table.game_data_man_addr = 0x3c4b218
		address_table.menu_man_imp = 0x3c58b90
		address_table.screen_state_offset = 0x718
	elseif ver.major == 1 and ver.minor == 3 and ver.build == 0 then
		address_table.event_flag_man_addr = 0x3c672a8
		address_table.game_data_man_addr = 0x3c5cd78
		address_table.menu_man_imp = 0x3c6a700
		address_table.screen_state_offset = 0x728
	elseif ver.major == 1 and ver.minor == 3 and ver.build == 1 then
		address_table.event_flag_man_addr = 0x3c672a8
		address_table.game_data_man_addr = 0x3c5cd78
		address_table.menu_man_imp = 0x3c6a700
		address_table.screen_state_offset = 0x728
	elseif ver.major == 1 and ver.minor == 3 and ver.build == 2 then
		address_table.event_flag_man_addr = 0x3c672a8
		address_table.game_data_man_addr = 0x3c5cd78
		address_table.menu_man_imp = 0x3c6a700
		address_table.screen_state_offset = 0x728
	elseif ver.major == 1 and ver.minor == 4 and ver.build == 0 then
		address_table.event_flag_man_addr = 0x3c0a538
		address_table.game_data_man_addr = 0x3c00028
		address_table.menu_man_imp = 0x3c0d9d0
		address_table.screen_state_offset = 0x728
	elseif ver.major == 1 and ver.minor == 4 and ver.build == 1 then
		address_table.event_flag_man_addr = 0x3c0a538
		address_table.game_data_man_addr = 0x3c00028
		address_table.menu_man_imp = 0x3c0d9d0
		address_table.screen_state_offset = 0x728
	elseif ver.major == 1 and ver.minor == 5 and ver.build == 0 then
		address_table.event_flag_man_addr = 0x3c222e8
		address_table.game_data_man_addr = 0x3c17ee8
		address_table.menu_man_imp = 0x3c25780
		address_table.screen_state_offset = 0x728
	elseif ver.major == 1 and ver.minor == 6 and ver.build == 0 then
		address_table.event_flag_man_addr = 0x3c33508
		address_table.game_data_man_addr = 0x3c29108
		address_table.menu_man_imp = 0x3c369a0
		address_table.screen_state_offset = 0x728
	elseif ver.major == 1 and ver.minor == 7 and ver.build == 0 then
		address_table.event_flag_man_addr = 0x3c4dec8
		address_table.game_data_man_addr = 0x3c43ac8
		address_table.menu_man_imp = 0x3c51360
		address_table.screen_state_offset = 0x728
	elseif ver.major == 1 and ver.minor == 8 and ver.build == 0 then
		address_table.event_flag_man_addr = 0x3cdbdf8
		address_table.game_data_man_addr = 0x3cd1948
		address_table.menu_man_imp = 0x3cdf140
		address_table.screen_state_offset = 0x728
	elseif ver.major == 1 and ver.minor == 8 and ver.build == 1 then
		address_table.event_flag_man_addr = 0x3cdbdf8
		address_table.game_data_man_addr = 0x3cd1948
		address_table.menu_man_imp = 0x3cdf140
		address_table.screen_state_offset = 0x728
	elseif ver.major == 1 and ver.minor == 9 and ver.build == 0 then
		address_table.event_flag_man_addr = 0x3cdf238
		address_table.game_data_man_addr = 0x3cd4d88
		address_table.menu_man_imp = 0x3ce2580
		address_table.screen_state_offset = 0x728
	elseif ver.major == 1 and ver.minor == 9 and ver.build == 1 then
		address_table.event_flag_man_addr = 0x3cdf238
		address_table.game_data_man_addr = 0x3cd4d88
		address_table.menu_man_imp = 0x3ce2580
		address_table.screen_state_offset = 0x728
	elseif ver.major == 2 and ver.minor == 0 and ver.build == 0 then
		address_table.event_flag_man_addr = 0x3cdf238
		address_table.game_data_man_addr = 0x3cd4d88
		address_table.menu_man_imp = 0x3ce2580
		address_table.screen_state_offset = 0x728
	else
		address_table.event_flag_man_addr = 0x3cdf238
		address_table.game_data_man_addr = 0x3cd4d88
		address_table.menu_man_imp = 0x3ce2580
		address_table.screen_state_offset = 0x728
	end
end

local function search_for_game_process()
	for pid, name in memreader.processes() do
		if name == 'eldenring.exe' then
			process = memreader.openprocess(pid)
			if process == nil then
				return
			end
			get_offset_table()
			break
		end
	end
end

function update_info_text()
	local source = obslua.obs_get_source_by_name(source_name)
	if source ~= nil then
		local hr = eldenring_igt / 3600000
		local min = (eldenring_igt % 3600000) / 60000
		local sec = (eldenring_igt % 60000) / 1000
		local cent = (eldenring_igt % 1000) / 10
		local text
		if hr == 0 then
			text = string.format("%02d:%02d.%02d", min, sec, cent)
		else
			text = string.format("%02d:%02d:%02d.%02d", hr, min, sec, cent)
		end
		obslua.obs_data_set_string(source_settings, "text", string.format(format_str, text))
		obslua.obs_source_update(source, source_settings)
		obslua.obs_source_release(source)
	end
end

function reset_button_clicked(props, p)
	eldenring_igt = 0
	return true
end

function script_properties()
	local props = obslua.obs_properties_create()

	local p = obslua.obs_properties_add_list(props, "source", "Text source", obslua.OBS_COMBO_TYPE_EDITABLE, obslua.OBS_COMBO_FORMAT_STRING)
	local sources = obslua.obs_enum_sources()
	if sources ~= nil then
		for _, source in ipairs(sources) do
			source_id = obslua.obs_source_get_unversioned_id(source)
			if source_id == "text_gdiplus" or source_id == "text_ft2_source" then
				local name = obslua.obs_source_get_name(source)
				obslua.obs_property_list_add_string(p, name, name)
			end
		end
	end
	obslua.source_list_release(sources)
	p = obslua.obs_properties_add_text(props, "format", "Time Format", obslua.OBS_TEXT_DEFAULT)
	obslua.obs_property_set_long_description(p, "'%s' will be replaced by in-game time")
	obslua.obs_properties_add_button(props, "reset_button", "Reset", reset_button_clicked)

	return props
end

function script_defaults(settings)
	obslua.obs_data_set_default_string(settings, "format", "IGT %s")
end

function script_update(settings)
	source_name = obslua.obs_data_get_string(settings, "source")
	format_str = obslua.obs_data_get_string(settings, "format")
	activated = false
	local source = obslua.obs_get_source_by_name(source_name)
	if source == nil then return end
	if not obslua.obs_source_is_hidden(source) then
		activated = true
	end
	obslua.obs_source_release(source)
end

function script_tick(sec)
	if activated == false then return end
	if process == nil then
		tick_countup = tick_countup + 1
		if tick_countup == 60 then
			search_for_game_process()
			tick_countup = 0
		end
		return
	end
	if process:exitcode() ~= nil then
		process = nil
		return
	end
	if address_table == nil then
		get_offset_table()
	end
	eldenring_in_game = process:readint_relative(address_table.menu_man_imp, address_table.screen_state_offset) == 0
	if not eldenring_in_game then
		return
	end

	eldenring_igt = process:readint_relative(address_table.game_data_man_addr, 0xA0)
	if eldenring_igt == nil then
		eldenring_igt = 0
	end
	update_info_text()
end

function activate_signal(cd, activating)
	local source = obslua.calldata_source(cd, "source")
	if source ~= nil then
		local name = obslua.obs_source_get_name(source)
		if name == source_name then
			activated = activating
			if activating then
				update_info_text()
			end
		end
	end
end

function source_activated(cd)
	activate_signal(cd, true)
end

function source_deactivated(cd)
	activate_signal(cd, false)
end

function script_load(settings)
	source_settings = obslua.obs_data_create()
	local sh = obslua.obs_get_signal_handler()
	obslua.signal_handler_connect(sh, "source_activate", source_activated)
	obslua.signal_handler_connect(sh, "source_deactivate", source_deactivated)
end

function script_unload()
	obslua.obs_data_release(source_settings)
end

function script_description()
	return "Display Elden Ring in-game time to a text source.\n\nMade by Soar Qin"
end
