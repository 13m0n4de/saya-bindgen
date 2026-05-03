# saya-bindgen

[English](./README.md) | [中文](./README.zh.md)

Generates [Saya](https://github.com/13m0n4de/saya) FFI bindings from C headers.

For example, given this C code:

```c
typedef enum {
    CAMERA_CUSTOM = 0,
    CAMERA_FREE,
    CAMERA_ORBITAL,
    CAMERA_FIRST_PERSON,
    CAMERA_THIRD_PERSON
} CameraMode;

typedef struct Image {
    void *data;
    int width;
    int height;
    int mipmaps;
    int format;
} Image;

void SetWindowIcons(Image *images, int count);
```

saya-bindgen produces:

```saya
pub type CameraMode = i32;
pub const CameraMode_CAMERA_CUSTOM: CameraMode = 0i32;
pub const CameraMode_CAMERA_FREE: CameraMode = 1i32;
pub const CameraMode_CAMERA_ORBITAL: CameraMode = 2i32;
pub const CameraMode_CAMERA_FIRST_PERSON: CameraMode = 3i32;
pub const CameraMode_CAMERA_THIRD_PERSON: CameraMode = 4i32;

pub struct Image {
    data: *opaque,
    width: i32,
    height: i32,
    mipmaps: i32,
    format: i32,
}

@symbol("SetWindowIcons") pub extern fn SetWindowIcons(images: *Image, count: i32) -> ();
```

> [!NOTE]
>
> - saya-bindgen is designed for single-header libraries like [Raylib](https://github.com/raysan5/raylib). Multi-header C libraries are not currently supported.
> - Macro expansion and parsing are handled by [chibicc](https://github.com/rui314/chibicc), which does not cover all C language features.

## Installation & Usage

Clone the repository:

```
git clone https://github.com/13m0n4de/saya-bindgen
```

Build and run with [Just](https://github.com/casey/just):

```
just run < raylib.h > raylib.saya
```
