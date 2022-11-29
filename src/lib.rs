use std::ffi::c_void;

#[repr(C)]
struct DecompressorInput {
    buffer: *mut c_void,
    length_in_bytes: u32,
}

#[repr(C)]
struct CompressorInput {
    buffer: *mut c_void,
    length_in_bytes: u32,
}

extern "C" {
    fn compress(input: *const CompressorInput, output: *mut DecompressorInput);
    fn decompress(input: *const DecompressorInput, output: *mut CompressorInput);
}

#[cfg(target_os = "windows")]
pub fn refpack_compress(input: &mut [u8]) -> Vec<u8> {
    unsafe {
        let mut input = CompressorInput {
            buffer: input.as_mut_ptr() as *mut c_void,
            length_in_bytes: input.len() as u32,
        };
        let mut output = DecompressorInput {
            buffer: 0 as *mut c_void, //immediately gets set as part of the function call
            length_in_bytes: 0,
        };
        compress(&mut input, &mut output);
        let cast_pointer = output.buffer as *mut u8;
        Vec::from_raw_parts(
            cast_pointer,
            output.length_in_bytes as usize,
            output.length_in_bytes as usize,
        )
    }
}

#[cfg(not(target_os = "windows"))]
pub fn refpack_compress(input: &mut [u8]) -> Vec<u8> {
    input.to_vec()
}

#[cfg(target_os = "windows")]
pub fn refpack_decompress(input: &mut [u8]) -> Vec<u8> {
    unsafe {
        let mut input = DecompressorInput {
            buffer: input.as_mut_ptr() as *mut c_void,
            length_in_bytes: input.len() as u32,
        };

        let mut output = CompressorInput {
            buffer: 0 as *mut c_void,
            length_in_bytes: 0,
        };
        decompress(&mut input, &mut output);
        let cast_pointer = output.buffer as *mut u8;
        Vec::from_raw_parts(
            cast_pointer,
            output.length_in_bytes as usize,
            output.length_in_bytes as usize,
        )
    }
}

#[cfg(not(target_os = "windows"))]
pub fn refpack_decompress(input: &mut [u8]) -> Vec<u8> {
    input.to_vec()
}

#[cfg(test)]
mod tests {
    use crate::{refpack_compress, refpack_decompress};

    #[test]
    fn compresses() {
        let mut input = b"Simple Test Input".to_vec();
        println!("input: {input:X?}");
        let mut compressed = refpack_compress(&mut input);
        println!("compressed: {compressed:X?}");
        let decompressed = refpack_decompress(&mut compressed);
        println!("decompressed: {decompressed:X?}");
    }

    #[test]
    fn decompress() {
        let mut input = vec![
            101, 0, 0, 0, 16, 251, 0, 0, 100, 137, 64, 0, 0, 224, 1, 0, 0, 2, 0, 6, 224, 2, 0, 1,
            1, 0, 11, 224, 1, 0, 2, 1, 2, 15, 0, 3, 224, 0, 0, 3, 1, 2, 6, 0, 4, 224, 4, 0, 1, 2,
            0, 29, 224, 2, 0, 3, 2, 0, 11, 224, 2, 0, 5, 0, 0, 2, 224, 1, 0, 5, 1, 2, 29, 0, 6,
            224, 5, 0, 2, 2, 0, 8, 224, 1, 0, 7, 0, 0, 6, 225, 2, 0, 7, 1, 0, 8, 0, 0, 254, 7, 0,
        ];

        println!("input: {input:X?}");

        let decompressed = refpack_decompress(&mut input);

        println!("Decompressed: {decompressed:X?}");
    }
}
