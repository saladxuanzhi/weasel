local module = dofile("Rime/lua/contextual_pair.lua")
local directory = assert(arg[1], "temporary directory required")
rime_api = { get_user_data_dir = function() return directory end }
local clock = os.time
local now = 1000
os.time = function() return now end
local function setup(enabled)
    local callback
    local ctx = {
        ascii = false,
        get_option = function(self) return self.ascii end,
        set_property = function(self, k, v) self[k] = v end,
        get_commit_text = function(self) return self.text end,
        commit_notifier = { connect = function(_, cb)
            callback = cb
            return { disconnect = function() callback = nil end }
        end }
    }
    local config = {
        get_bool = function() return enabled end,
        get_int = function() return nil end
    }
    local env = { engine = { context = ctx, schema = { config = config } } }
    module.init(env)
    return env, function(text) ctx.text = text; if callback then callback(ctx) end end
end
local function cand(text, finish, kind)
    return { text = text, start = 0, _end = finish or 5, type = kind or "phrase" }
end
local function query(env, candidates)
    local result = {}
    yield = function(c) result[#result + 1] = c end
    local stream = { index = 0 }
    stream.iter = function(self)
        -- librime-lua returns both next(translation) and the translation object.
        return function(state)
            assert(state == stream, "missing Translation iterator state")
            state.index = state.index + 1
            return candidates[state.index]
        end, self
    end
    module.func(stream, env)
    assert(#result == #candidates, "lost or duplicated candidate")
    return result
end
local env, commit = setup(true)
local model, mold, partial = cand("模型"), cand("磨型"), cand("模", 2)
commit("部署"); commit("模型"); commit("。"); commit("部署")
assert(query(env, {mold, partial, model})[1] == model, "learned pair must lead")
assert(env.engine.context.contextual_pair_preferred == "模型", "P2C priority signal")
commit("研究")
assert(query(env, {mold, model})[1] == mold, "different context must keep order")
commit("部署"); now = now + 61
assert(query(env, {mold, model})[1] == mold, "expired context")
commit("部署"); commit("abc")
assert(query(env, {mold, model})[1] == mold, "non-Chinese must reset context")
commit("部署"); env.engine.context.ascii = true
assert(query(env, {mold, model})[1] == mold, "ASCII mode")
module.fini(env)
env, commit = setup(true)
commit("部署")
local result = query(env, {mold, partial, model, cand("符号", 5, "punct")})
assert(result[1] == model and result[2] == partial and result[4].type == "punct",
    "reload, segment boundaries and special slots")
local many = {}
for i = 1, 50 do many[i] = cand("候选") end
query(env, many)
module.fini(env)
env, commit = setup(false)
assert(query(env, {mold, model})[1] == mold, "disabled mode")
module.fini(env)
os.time = clock
print("contextual_pair: all tests passed")
