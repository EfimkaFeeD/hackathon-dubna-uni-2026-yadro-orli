use libloading::{Library, Symbol};
use std::ffi::c_void;
use std::sync::OnceLock;
use crate::models::{AudioSpec, ChunkType};

// Загружаем C++ библиотеку глобально один раз
static PLUGIN_LIB: OnceLock<Library> = OnceLock::new();

pub fn init_plugin(plugin_path: &str) -> anyhow::Result<()> {
    unsafe {
        let lib = Library::new(plugin_path)?;
        PLUGIN_LIB.set(lib).map_err(|_| anyhow::anyhow!("Плагин уже инициализирован"))?;
    }
    Ok(())
}

#[repr(C)]
pub struct CAudioSpec {
    pub sample_rate: u32,
    pub is_mono: i32,
    pub bits_per_sample: u8,
    pub is_signed: i32,
    pub is_float: i32,
}

#[repr(C)]
pub struct FfiPluginResult {
    pub packet_num: u32,
    pub chunk_type: u8,
}

type CreateFn = unsafe extern "C" fn(spec: *const CAudioSpec, margin_db: f32, vad_mode: i32, noise_window_ms: i32) -> *mut c_void;
type ProcessFn = unsafe extern "C" fn(handle: *mut c_void, packet_num: u32, buf: *const u8, len: usize) -> FfiPluginResult;
type DestroyFn = unsafe extern "C" fn(handle: *mut c_void);

pub struct AudioProcessor {
    handle: *mut c_void,
}

unsafe impl Send for AudioProcessor {}

impl AudioProcessor {
    pub fn new(spec: &AudioSpec) -> anyhow::Result<Self> {
        let lib = PLUGIN_LIB.get().ok_or_else(|| anyhow::anyhow!("Библиотека не загружена"))?;

        let c_spec = CAudioSpec {
            sample_rate: spec.sample_rate,
            is_mono: if spec.is_mono() { 1 } else { 0 },
            bits_per_sample: spec.bits_per_sample,
            is_signed: if spec.is_signed { 1 } else { 0 },
            is_float: 0,
        };

        unsafe {
            let create_fn: Symbol<CreateFn> = lib.get(b"processor_create")?;
            let handle = create_fn(&c_spec, 0.0, -1, -1);
            if handle.is_null() {
                return Err(anyhow::anyhow!("C++ вернул null"));
            }
            Ok(Self { handle })
        }
    }

    pub fn process(&self, packet_num: u32, data: &[u8]) -> anyhow::Result<ChunkType> {
        let lib = PLUGIN_LIB.get().ok_or_else(|| anyhow::anyhow!("Библиотека не загружена"))?;
        unsafe {
            let process_fn: Symbol<ProcessFn> = lib.get(b"processor_process")?;
            let res = process_fn(self.handle, packet_num, data.as_ptr(), data.len());
            // Превращаем u8 в ChunkType!
            Ok(ChunkType::from(res.chunk_type))
        }
    }
}

impl Drop for AudioProcessor {
    fn drop(&mut self) {
        if let Some(lib) = PLUGIN_LIB.get() {
            unsafe {
                if let Ok(destroy_fn) = lib.get::<Symbol<DestroyFn>>(b"processor_destroy") {
                    destroy_fn(self.handle);
                }
            }
        }
    }
}