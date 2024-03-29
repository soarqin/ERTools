return {
    -- Output folder for txt files
    output_folder = 'output/',
    -- Interval for memory reading, in milliseconds
    update_interval = 1000,
    -- Set plugins to load (set to false or remove certain items to disable them)
    plugins = {
        info = true,
        bosses = true,
        items = true,
    },
    -- language, check `data` folder for language list
    language = 'zhocn',
    -- Config entries for plugins
    bosses = {
        -- Show only rememberance bosses
        only_rememberance = true,
    },
    items = {
        -- Show weapon collection progress
        show_weapon_collections = false,
        -- Show protector collection progress
        show_protector_collections = false,
        -- Show equipped items
        show_equipped = true,
    }
}
