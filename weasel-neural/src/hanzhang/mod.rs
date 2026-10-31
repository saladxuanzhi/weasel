//! 含章：自训的字级 Transformer（[`CharLm`]）与按拼音逐字生成整句的 [`P2c`]。
//!
//! 本目录的代码来自 `qingjian/crates/qingjian-neural/src/hanzhang/`，只删掉了接
//! `qingjian-core` 的 `core_scorer.rs`（那是青简自家引擎的适配层），其余原样保留，
//! 便于日后与上游对账。

mod char_lm;
mod config;
mod p2c;
pub mod qjm;
mod scorer;
mod vocab;

// PrefixCache 现由 scorer 内部用；对外留着，第二期的增量前缀缓存要用它
#[allow(unused_imports)]
pub use char_lm::{CharLm, PrefixCache};
pub use config::ModelConfig;
pub use p2c::{Candidate, P2c};
pub use qjm::find_model;
pub use scorer::CharScorer;
pub use vocab::Vocab;
