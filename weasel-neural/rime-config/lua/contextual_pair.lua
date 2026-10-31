-- 按上一次完整上屏的中文片段学习搭配；在真实候选流内排序。
local M = {}

local function chinese(text)
    if text == "" then return false end
    for _, c in utf8.codes(text) do
        if not ((c >= 0x3400 and c <= 0x9fff) or
                (c >= 0x20000 and c <= 0x323af)) then return false end
    end
    return utf8.len(text) <= 32
end

local function key(before, text) return before .. "\t" .. text end

function M.init(env)
    local config = env.engine.schema.config
    env.enabled = config:get_bool("contextual_pair/enabled") == true
    env.timeout = config:get_int("contextual_pair/timeout_seconds") or 60
    env.limit = math.max(1, math.min(64, config:get_int("contextual_pair/candidate_limit") or 24))
    env.counts, env.previous, env.last_commit = {}, "", 0
    if not env.enabled then return end
    env.path = rime_api.get_user_data_dir() .. "/contextual_pair.tsv"
    local file = io.open(env.path, "r")
    if file then
        for line in file:lines() do
            local before, text = line:match("^([^\t]+)\t([^\t]+)$")
            if before and chinese(before) and chinese(text) then
                local k = key(before, text)
                env.counts[k] = math.min(1000000, (env.counts[k] or 0) + 1)
            end
        end
        file:close()
    end
    env.connection = env.engine.context.commit_notifier:connect(function(ctx)
        ctx:set_property("contextual_pair_preferred", "")
        local text, now = ctx:get_commit_text(), os.time()
        if chinese(text) and env.previous ~= "" and now - env.last_commit <= env.timeout then
            local k = key(env.previous, text)
            env.counts[k] = math.min(1000000, (env.counts[k] or 0) + 1)
            local out = io.open(env.path, "a")
            if out then out:write(k, "\n"); out:close() end
        end
        env.previous = chinese(text) and text or ""
        env.last_commit = now
    end)
end

function M.func(input, env)
    env.engine.context:set_property("contextual_pair_preferred", "")
    local iter = input:iter()
    if not env.enabled or env.previous == "" or
            os.time() - env.last_commit > env.timeout or
            env.engine.context:get_option("ascii_mode") then
        for cand in iter do yield(cand) end
        return
    end
    local candidates = {}
    for cand in iter do
        candidates[#candidates + 1] = cand
        if #candidates >= env.limit then break end
    end
    local first, slots, ranked = candidates[1], {}, {}
    for i, cand in ipairs(candidates) do
        -- 不让短片段抢整句，也不改变日期、符号等特殊候选的位置。
        if cand.start == first.start and cand._end == first._end and chinese(cand.text) and
                (cand.type == "phrase" or cand.type == "user_phrase" or cand.type == "sentence") then
            slots[#slots + 1] = i
            ranked[#ranked + 1] = { cand = cand, rank = i,
                count = env.counts[key(env.previous, cand.text)] or 0 }
        end
    end
    table.sort(ranked, function(a, b)
        if a.count == b.count then return a.rank < b.rank end
        return a.count > b.count
    end)
    for i, slot in ipairs(slots) do candidates[slot] = ranked[i].cand end
    if slots[1] == 1 and ranked[1].count > 0 then
        env.engine.context:set_property("contextual_pair_preferred", candidates[1].text)
    end
    for _, cand in ipairs(candidates) do yield(cand) end
    for cand in iter do yield(cand) end
end

function M.fini(env)
    if env.connection then env.connection:disconnect() end
end

return M
