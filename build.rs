fn main() {
    #[cfg(target_os = "windows")]
    cc::Build::new().file("c_lib/refpack.c").compile("refpack");
}
