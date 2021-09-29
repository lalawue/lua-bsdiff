
# About

memory bsdiff / bspatch lib for Lua, no compress, you can compress it by another library.

# Usage

```lua
local bsdiff = require("bsdiff")

local old_content = [[..anything..]]
local new_content = [[..anything...]]

-- generate patch
local patch, emsg = bsdiff.diff(old_content, new_content)
if emsg then
    print("diff error: ", emsg)
    os.exit(0)
end
print("patch length: ", patch:len())

-- generate another new_content
local out_content, emsg = bsdiff.patch(old_content, patch)
if emsg then
    print("patch error: ", emsg)
    os.exit(0)
end

print("content equal: ", new_content == out_content)
```
