// SPDX-License-Identifier: GPL-2.0

//! Rust out-of-tree sample

use kernel::prelude::*;

module! {
    type: RustModuleStarter,
    name: "my_rust",
    author: "Michael Harris <michaelharriscode@gmail.com>",
    description: "This is my rust module starter",
    license: "GPL",
}

struct RustModuleStarter; 

impl kernel::Module for RustModuleStarter {
    fn init(_module: &'static ThisModule) -> Result<Self> {
        pr_info!("Rust Module Starter: Initialized!\n");

        Ok(RustModuleStarter)
    }
}

impl Drop for RustModuleStarter {
    fn drop(&mut self) {
        pr_info!("Rust Module Starter: DROPPED!!!!!\n");
    }
}
