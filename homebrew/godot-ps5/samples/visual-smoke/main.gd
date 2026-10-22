extends Node2D

# REAL GODOT 4.7.2 scene authored to exercise the upstream CanvasItem pipeline.
# Standalone GPU probes never run this script, keeping native-vs-Godot clear.
var elapsed: float = 0.0
var previous_cross: bool = false
var sprite: Sprite2D
var status_label: Label
var background: ColorRect
var count: int = 0

func _ready() -> void:
    background = ColorRect.new()
    background.color = Color(0.06, 0.10, 0.18, 1.0)
    background.position = Vector2.ZERO
    background.size = Vector2(1920, 1080)
    add_child(background)

    var img: Image = Image.create_empty(64, 64, false, Image.FORMAT_RGBA8)
    for y in range(64):
        for x in range(64):
            var checker: bool = ((x / 8 + y / 8) % 2) == 0
            img.set_pixel(x, y, Color(1.0, 0.10, 0.70, 0.65) if checker else Color(0.05, 0.95, 0.95, 0.65))
    sprite = Sprite2D.new()
    sprite.name = "PreviewSprite"
    sprite.texture = ImageTexture.create_from_image(img)
    sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
    sprite.position = Vector2(960, 520)
    sprite.scale = Vector2(7.0, 7.0)
    add_child(sprite)

    status_label = Label.new()
    status_label.text = "Godot 4.7.2 - REAL PS5 Canvas2D"
    status_label.position = Vector2(70, 45)
    status_label.add_theme_font_size_override("font_size", 42)
    add_child(status_label)
    print("GODOT_PS5_REAL_2D_SCENE_READY")

func _process(delta: float) -> void:
    elapsed += delta
    sprite.rotation += delta * 0.65
    sprite.position.x = 960.0 + sin(elapsed * 0.85) * 360.0
    var cross: bool = Input.is_joy_button_pressed(0, JOY_BUTTON_A)
    if cross and not previous_cross:
        count += 1
        background.color = Color(0.18, 0.07, 0.08, 1.0) if count % 2 == 1 else Color(0.06, 0.10, 0.18, 1.0)
        status_label.text = "Godot PS5 Canvas2D  X pressed: %d" % count
        print("GODOT_PS5_REAL_2D_CROSS_PRESSED")
    previous_cross = cross
    if elapsed >= 20.0:
        print("GODOT_PS5_REAL_2D_SCENE_FINISHED")
        get_tree().quit(0)
