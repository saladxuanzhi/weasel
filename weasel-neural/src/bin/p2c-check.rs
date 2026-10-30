//! 验证用 CLI：加载模型，把拼音转成整句，并给出延迟。
//!
//! ```text
//! p2c-check <model.qjm> <拼音...>
//! p2c-check model.qjm jintiantianqihenhao shurukuang weixiazaihaole
//! ```

use std::process::ExitCode;
use std::time::Instant;

use weasel_p2c::HanZhangP2c;

fn main() -> ExitCode {
    let mut args = std::env::args_os().skip(1);
    let Some(model_path) = args.next() else {
        eprintln!("用法: p2c-check <model.qjm> <拼音...>");
        return ExitCode::from(2);
    };
    let keys: Vec<String> = args
        .map(|a| a.to_string_lossy().into_owned())
        .filter(|a| !a.is_empty())
        .collect();
    if keys.is_empty() {
        eprintln!("请至少给一个拼音串");
        return ExitCode::from(2);
    }

    let t = Instant::now();
    let model = match HanZhangP2c::load(&model_path) {
        Ok(m) => m,
        Err(e) => {
            eprintln!("加载失败: {e}");
            return ExitCode::FAILURE;
        }
    };
    println!("模型: {}  加载 {:?}", model.model_name(), t.elapsed());

    let mut worst = std::time::Duration::ZERO;
    let mut total = std::time::Duration::ZERO;
    let mut runs = 0usize;

    for key in &keys {
        // 每次跑两遍：第一遍吃冷缓存，第二遍是稳态延迟
        for round in 1..=2 {
            let t = Instant::now();
            match model.convert(key, 5, 0) {
                Ok(items) => {
                    let dt = t.elapsed();
                    if round == 2 {
                        total += dt;
                        worst = worst.max(dt);
                        runs += 1;
                    }
                    if round == 2 {
                        println!("\n{key}  ({:?}, {} 条)", dt, items.len());
                        for (i, item) in items.iter().enumerate() {
                            println!("  {}. {:<24} {:>10.2}", i + 1, item.text, item.score);
                        }
                    }
                }
                Err(e) => {
                    println!("\n{key}  失败: {e}");
                    break;
                }
            }
        }
    }

    if runs > 0 {
        println!(
            "\n稳态延迟: 平均 {:?} / 最慢 {:?}（{} 次）",
            total / runs as u32,
            worst,
            runs
        );
    }
    ExitCode::SUCCESS
}
