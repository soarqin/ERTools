-- util functions
local function read_file(path)
    local file = io.open(path, 'rb')
    if not file then return nil end
    local content = file:read('*a')
    file:close()
    return content
end

return {
    read_file = read_file
}