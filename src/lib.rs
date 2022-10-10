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

extern {
    fn compress(input: *const CompressorInput, output: *mut DecompressorInput);
    fn decompress(input: *const DecompressorInput, output: *mut CompressorInput);
}

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
        Vec::from_raw_parts(cast_pointer, output.length_in_bytes as usize, output.length_in_bytes as usize)
    }
}

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
        Vec::from_raw_parts(cast_pointer, output.length_in_bytes as usize, output.length_in_bytes as usize)
    }
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

}
