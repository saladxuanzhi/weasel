//! 逐键延迟：模拟用户一个字母一个字母敲，每键都调一次 convert，看延迟曲线。
//!
//! 这决定了 weasel 侧要不要防抖、阈值放多少。
//!
//! ```text
//! p2c-typing <model.qjm> jintiantianqihenhao
//! ```

use std::process::ExitCode;
use std::time::Instant;

use weasel_p2c::HanZhangP2c;

fn main() -> ExitCode {
    let mut args = std::env::args_os().skip(1);
    let Some(model_path) = args.next() else {
        eprintln!("用法: p2c-typing <model.qjm> <拼音> [beam]");
        return ExitCode::from(2);
    };
    let Some(full) = args.next().map(|a| a.to_string_lossy().into_owned()) else {
        eprintln!("请给一段完整拼音");
        return ExitCode::from(2);
    };
    let beam: usize = args
        .next()
        .and_then(|a| a.to_string_lossy().parse().ok())
        .unwrap_or(5);

    let t = Instant::now();
    let model = match HanZhangP2c::load(&model_path) {
        Ok(m) => m,
        Err(e) => {
            eprintln!("加载失败: {e}");
            return ExitCode::FAILURE;
        }
    };
    println!("加载 {:?}  beam={beam}\n", t.elapsed());
    println!(
        "{:>3} {:<26} {:>4} {:>10}",
        "键", "已输入的拼音", "条数", "延迟"
    );

    let mut worst = std::time::Duration::ZERO;
    let mut total = std::time::Duration::ZERO;
    let mut runs = 0usize;
    let mut first = String::new();

    for (i, ch) in full.chars().enumerate() {
        first.push(ch);
        let t = Instant::now();
        match model.convert(&first, beam, 0) {
            Ok(items) => {
                let dt = t.elapsed();
                total += dt;
                worst = worst.max(dt);
                runs += 1;
                println!(
                    "{:>3} {:<26} {:>4} {:>8.1} ms  → {}",
                    i + 1,
                    first,
                    items.len(),
                    dt.as_secs_f64() * 1000.0,
                    items.first().map(|c| c.text.as_str()).unwrap_or("")
                );
            }
            Err(e) => println!("{:>3} {:<26} 失败: {e}", i + 1, first),
        }
    }

    if runs > 0 {
        println!(
            "\n每键平均 {:.1} ms / 最慢 {:.1} ms（{} 键）",
            total.as_secs_f64() * 1000.0 / runs as f64,
            worst.as_secs_f64() * 1000.0,
            runs
        );
    }
    ExitCode::SUCCESS
}
