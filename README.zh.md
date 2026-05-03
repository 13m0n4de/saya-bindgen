# saya-bindgen

[English](./README.md) | [中文](./README.zh.md)

为 C 语言库自动生成 [Saya](https://github.com/13m0n4de/saya) FFI 绑定。

例如这段 C 语言代码：

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

生成以下 Saya 代码：

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
> - saya-bindgen 专门为单头文件库开发，比如 [Raylib](https://github.com/raysan5/raylib)，暂不支持多头文件的 C 语言库。
> - 宏展开和代码解析使用 [chibicc](https://github.com/rui314/chibicc)，无法满足所有 C 语言特性。

## 安装 & 使用

克隆仓库：

```
git clone https://github.com/13m0n4de/saya-bindgen
```

使用 [Just](https://github.com/casey/just) 编译并运行：

```
just run
```
