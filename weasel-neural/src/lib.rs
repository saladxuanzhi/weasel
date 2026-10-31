//! 把青简的「含章·通变」P2C 整句模型嵌进 weasel 的验证工程。
//!
//! 只做一件事：给一段拼音，让模型直接造出整句候选。不含候选重排（那是第二期），
//! 不依赖青简的 Engine / 词库 / 学习数据。
//!
//! 两层接口：
//! - [`HanZhangP2c`]：给 Rust 侧用的安全封装
//! - 底部的 `extern "C"` 函数：给 weasel（C++）用的 C ABI，见 `include/p2c.h`

mod device;
pub mod error;
pub mod format;
mod hanzhang;

pub use error::NeuralError;
pub use hanzhang::{Candidate, CharScorer, P2c};

use std::path::Path;
use std::sync::Arc;

/// 加载好的整句模型。克隆的是同一份权重，可以跨线程共享（内部已是 `Sync`）。
#[derive(Clone)]
pub struct HanZhangP2c {
    scorer: Arc<CharScorer>,
}

/// 一条整句候选。
#[derive(Debug, Clone)]
pub struct Generated {
    pub text: String,
    /// 生成这段汉字的 log 概率之和，越接近 0 越好。同一次调用内可比。
    pub score: f64,
}

impl HanZhangP2c {
    /// 加载 `.qjm` 单文件、三件套目录，或装着其中之一的目录。
    pub fn load(path: impl AsRef<Path>) -> Result<Self, NeuralError> {
        Ok(Self {
            scorer: Arc::new(CharScorer::load(path.as_ref())?),
        })
    }

    /// 拼音键 → 前 `beam` 条整句，按分数降序。
    ///
    /// `keys` 是不带分隔的小写全拼（`jintiantianqihenhao`）；`max_chars` 是生成长度上限，
    /// 给 0 表示按按键长度估算（每个音节平均 2 个字母，留 2 倍余量）。
    pub fn convert(
        &self,
        keys: &str,
        beam: usize,
        max_chars: usize,
    ) -> Result<Vec<Generated>, NeuralError> {
        let decoder = P2c::new(self.scorer.model(), self.scorer.vocab())
            .ok_or_else(|| NeuralError::Corrupt("这个模型不是 P2C 模型（字表里没有 <sep>）"))?;
        let max_chars = if max_chars == 0 {
            // 音节数上限按「全是单字母音节」估：a/o/e 这种极端情况下也不会截断
            keys.len().clamp(1, 64)
        } else {
            max_chars
        };
        let items = decoder.convert(keys, beam, max_chars)?;
        Ok(items
            .into_iter()
            .map(|c| Generated {
                text: c.text,
                score: c.score,
            })
            .collect())
    }

    /// 字表里有没有这些字——没有字表外字的句子才算得准，调用方可以先筛。
    pub fn knows(&self, text: &str) -> bool {
        self.scorer.vocab().knows(text)
    }

    pub fn model_name(&self) -> String {
        self.scorer
            .metadata()
            .map(|m| m.name.clone())
            .unwrap_or_default()
    }
}

// ---------------------------------------------------------------------------
// C ABI：给 weasel 的 C++ 侧用
// ---------------------------------------------------------------------------

use std::ffi::{c_char, c_double, c_int, CStr, CString};

/// 句柄。不透明，C++ 侧只当指针用。
pub struct P2cHandle(HanZhangP2c);

/// 一条候选。`text` 是 UTF-8，由 `p2c_candidates_free` 释放。
#[repr(C)]
pub struct P2cCandidate {
    pub text: *mut c_char,
    pub score: c_double,
}

thread_local! {
    static LAST_ERROR: std::cell::RefCell<CString> =
        std::cell::RefCell::new(CString::new("").unwrap());
}

fn set_error(msg: impl Into<String>) {
    let msg = msg.into().replace('\0', " ");
    LAST_ERROR.with(|e| *e.borrow_mut() = CString::new(msg).unwrap_or_default());
}

/// 最近一次失败的原因（UTF-8，空串表示没有错误）。指针在下次出错前有效。
#[no_mangle]
pub extern "C" fn p2c_last_error() -> *const c_char {
    LAST_ERROR.with(|e| e.borrow().as_ptr())
}

/// 加载模型。`model_path` 是 UTF-8 的窄字符串（Windows 上请先转成 UTF-8 再传，
/// 不要传 ACP 编码）。成功返回句柄，失败返回 NULL 并可用 [`p2c_last_error`] 取原因。
///
/// 这一步要读 45 MB 权重并建图，实测数百毫秒到数秒，**不要放在按键路径上**。
    /// # Safety
    /// `model_path` 必须是 `'\0'` 结尾的合法 UTF-8 C 字符串指针（或 NULL，返回 NULL）。
#[no_mangle]
pub unsafe extern "C" fn p2c_load(model_path: *const c_char) -> *mut P2cHandle {
    if model_path.is_null() {
        set_error("p2c_load: model_path 是空指针");
        return std::ptr::null_mut();
    }
    let path = match CStr::from_ptr(model_path).to_str() {
        Ok(s) => s,
        Err(_) => {
            set_error("p2c_load: 路径不是合法 UTF-8");
            return std::ptr::null_mut();
        }
    };
    match HanZhangP2c::load(path) {
        Ok(model) => Box::into_raw(Box::new(P2cHandle(model))),
        Err(e) => {
            set_error(format!("p2c_load({path}): {e}"));
            std::ptr::null_mut()
        }
    }
}

/// 释放 [`p2c_load`] 返回的句柄。传 NULL 是安全的。
    /// # Safety
    /// `handle` 必须是 [`p2c_load`] 返回且尚未释放的句柄，或 NULL。
    /// 释放后不得再使用。
#[no_mangle]
pub unsafe extern "C" fn p2c_free(handle: *mut P2cHandle) {
    if !handle.is_null() {
        drop(Box::from_raw(handle));
    }
}

/// 拼音 → 整句候选。
///
/// `keys` 是 UTF-8 的不带分隔小写全拼；`beam` 建议 5；`max_chars` 给 0 表示自动。
/// 成功时把候选数组写进 `*out`、返回条数（≥0），调用方用
/// [`p2c_candidates_free`] 释放。失败返回 -1，`*out` 不动。
    /// # Safety
    /// - `handle` 必须是 [`p2c_load`] 返回的有效句柄
    /// - `keys` 必须是 `'\0'` 结尾的合法 UTF-8 C 字符串
    /// - `out` 必须是可写的 `*mut *mut P2cCandidate`，且函数成功时其原值会被覆盖
#[no_mangle]
pub unsafe extern "C" fn p2c_convert(
    handle: *mut P2cHandle,
    keys: *const c_char,
    beam: c_int,
    max_chars: c_int,
    out: *mut *mut P2cCandidate,
) -> c_int {
    if handle.is_null() || keys.is_null() || out.is_null() {
        set_error("p2c_convert: 空指针");
        return -1;
    }
    let model = &(*handle).0;
    let keys = match CStr::from_ptr(keys).to_str() {
        Ok(s) => s,
        Err(_) => {
            set_error("p2c_convert: 拼音不是合法 UTF-8");
            return -1;
        }
    };
    let beam = beam.max(1) as usize;
    let max_chars = max_chars.max(0) as usize;
    let items = match model.convert(keys, beam, max_chars) {
        Ok(v) => v,
        Err(e) => {
            set_error(format!("p2c_convert({keys}): {e}"));
            return -1;
        }
    };
    let mut boxed: Vec<P2cCandidate> = items
        .into_iter()
        .map(|g| P2cCandidate {
            text: CString::new(g.text).unwrap_or_default().into_raw(),
            score: g.score,
        })
        .collect();
    let n = boxed.len() as c_int;
    boxed.shrink_to_fit();
    *out = boxed.as_mut_ptr();
    std::mem::forget(boxed);
    n
}

/// 释放 [`p2c_convert`] 写出的候选数组（连同每条候选的 `text`）。
    /// # Safety
    /// `candidates` 必须是 [`p2c_convert`] 写出、尚未释放的数组，`n` 必须等于当时返回的条数。
    /// 同一个数组不能释放两次。
#[no_mangle]
pub unsafe extern "C" fn p2c_candidates_free(candidates: *mut P2cCandidate, n: c_int) {
    if candidates.is_null() || n <= 0 {
        return;
    }
    let v = Vec::from_raw_parts(candidates, n as usize, n as usize);
    for c in v {
        if !c.text.is_null() {
            drop(CString::from_raw(c.text));
        }
    }
}

/// 字表里有没有这些字。0 = 没有（或出错），1 = 有。用于筛掉算不准的候选。
    /// # Safety
    /// `handle` 必须是有效句柄（或 NULL），`text` 必须是 `'\0'` 结尾的合法 UTF-8 C 字符串（或 NULL）。
#[no_mangle]
pub unsafe extern "C" fn p2c_knows(handle: *mut P2cHandle, text: *const c_char) -> c_int {
    if handle.is_null() || text.is_null() {
        return 0;
    }
    let model = &(*handle).0;
    match CStr::from_ptr(text).to_str() {
        Ok(s) => c_int::from(model.knows(s)),
        Err(_) => 0,
    }
}
