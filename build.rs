fn main() {
    cc::Build::new().file("c_lib/refpack.c").compile("refpack");
}
